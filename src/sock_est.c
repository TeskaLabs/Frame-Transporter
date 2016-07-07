#include "all.h"

// TODO: Conceptual: Consider implementing readv() and writev() versions for more complicated frame formats
// TODO: Consider adding a support for HTTPS proxy

static void established_socket_on_read(struct ev_loop * loop, struct ev_io * watcher, int revents);
static void established_socket_on_write(struct ev_loop * loop, struct ev_io * watcher, int revents);

enum read_event
{
	READ_WANT_READ = 1,
	SSL_HANDSHAKE_WANT_READ = 2,
	SSL_SHUTDOWN_WANT_READ = 4,
	SSL_WRITE_WANT_READ = 8,
};

enum write_event
{
	WRITE_WANT_WRITE = 1,
	SSL_HANDSHAKE_WANT_WRITE = 2,
	SSL_SHUTDOWN_WANT_WRITE = 4,
	SSL_READ_WANT_WRITE = 8,
	CONNECT_WANT_WRITE = 16,
};

static void established_socket_read_set_event(struct established_socket * this, enum read_event event);
static void established_socket_read_unset_event(struct established_socket * this, enum read_event event);
static void established_socket_write_set_event(struct established_socket * this, enum write_event event);
static void established_socket_write_unset_event(struct established_socket * this, enum write_event event);

static void established_socket_on_write_event(struct established_socket * this);
static void established_socket_on_ssl_sent_shutdown_event(struct established_socket * this);

///

#define TRACE_FMT "fd:%d Rs:%c Rp:%c Rt:%c Re:%c%c%c%c Rw:%c Ws:%c Wo:%c Wr:%c We:%c%c%c%c%c Ww:%c c:%c a:%c ssl:%d%c E:%d"
#define TRACE_ARGS \
	this->read_watcher.fd, \
	(this->flags.read_shutdown) ? 'Y' : '.', \
	(this->flags.read_partial) ? 'Y' : '.', \
	(this->flags.read_throttle) ? 'Y' : '.', \
	(this->read_events & READ_WANT_READ) ? 'R' : '.', \
	(this->read_events & SSL_HANDSHAKE_WANT_READ) ? 'H' : '.', \
	(this->read_events & SSL_SHUTDOWN_WANT_READ) ? 'S' : '.', \
	(this->read_events & SSL_WRITE_WANT_READ) ? 'W' : '.', \
	(ev_is_active(&this->read_watcher)) ? 'Y' : '.', \
	(this->flags.write_shutdown) ? 'Y' : '.', \
	(this->flags.write_open) ? 'Y' : '.', \
	(this->flags.write_ready) ? 'Y' : '.', \
	(this->write_events & WRITE_WANT_WRITE) ? 'W' : '.', \
	(this->write_events & SSL_HANDSHAKE_WANT_WRITE) ? 'H' : '.', \
	(this->write_events & SSL_SHUTDOWN_WANT_WRITE) ? 'S' : '.', \
	(this->write_events & SSL_READ_WANT_WRITE) ? 'R' : '.', \
	(this->write_events & CONNECT_WANT_WRITE) ? 'c' : '.', \
	(ev_is_active(&this->write_watcher)) ? 'Y' : '.', \
	(this->flags.connecting) ? 'Y' : '.', \
	(this->flags.active) ? 'Y' : '.', \
	this->flags.ssl_status, this->ssl == NULL ? '.' : '*', \
	this->syserror

///

static bool established_socket_init(struct established_socket * this, struct established_socket_cb * cbs, int fd, struct context * context, const struct sockaddr * peer_addr, socklen_t peer_addr_len, int ai_family, int ai_socktype, int ai_protocol)
{
	assert(this != NULL);
	assert(cbs != NULL);

	// Disable Nagle
#ifdef TCP_NODELAY
	int flag = 1;
	int rc_tcp_nodelay = setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(flag) );
	if (rc_tcp_nodelay == -1) L_WARN_ERRNO(errno, "Couldn't setsockopt on client connection (TCP_NODELAY)");
#endif

	// Set Congestion Window size
#ifdef TCP_CWND
	int cwnd = 10; // from SPDY best practicies
	int rc_tcp_cwnd = setsockopt(fd, IPPROTO_TCP, TCP_CWND, &cwnd, sizeof(cwnd));
	if (rc_tcp_cwnd == -1) L_WARN_ERRNO(errno, "Couldn't setsockopt on client connection (TCP_CWND)");
#endif

	bool res = set_socket_nonblocking(fd);
	if (!res) L_WARN_ERRNO(errno, "Failed when setting established socket to non-blocking mode");

	this->data = NULL;
	this->cbs = cbs;
	this->context = context;
	this->syserror = 0;
	this->read_frame = NULL;
	this->write_frames = NULL;
	this->write_frame_last = &this->write_frames;
	this->stats.read_events = 0;
	this->stats.write_events = 0;
	this->stats.write_direct = 0;
	this->stats.read_bytes = 0;
	this->stats.write_bytes = 0;
	this->flags.read_partial = false;
	this->flags.read_shutdown = false;
	this->read_shutdown_at = NAN;
	this->flags.write_shutdown = false;
	this->write_shutdown_at = NAN;
	this->flags.write_open = true;
	this->flags.ssl_status = 0;
	this->flags.read_throttle = false;	
	this->ssl = NULL;

	assert(this->context->ev_loop != NULL);
	this->created_at = ev_now(this->context->ev_loop);

	memcpy(&this->ai_addr, peer_addr, peer_addr_len);
	this->ai_addrlen = peer_addr_len;

	this->ai_family = ai_family;
	this->ai_socktype = ai_socktype;
	this->ai_protocol = ai_protocol;

	ev_io_init(&this->read_watcher, established_socket_on_read, fd, EV_READ);
	ev_set_priority(&this->read_watcher, -1); // Read has always lower priority than writing
	this->read_watcher.data = this;
	this->read_events = 0;

	ev_io_init(&this->write_watcher, established_socket_on_write, fd, EV_WRITE);
	ev_set_priority(&this->write_watcher, 1);
	this->write_watcher.data = this;
	this->write_events = 0;
	this->flags.write_ready = false;

	return true;
}

bool established_socket_init_accept(struct established_socket * this, struct established_socket_cb * cbs, struct listening_socket * listening_socket, int fd, const struct sockaddr * peer_addr, socklen_t peer_addr_len)
{
	assert(listening_socket != NULL);

	L_TRACE(L_TRACEID_SOCK_STREAM, "fd:%d", fd);

	bool ok = established_socket_init(
		this,
		cbs,
		fd,
		listening_socket->context, 
		peer_addr, peer_addr_len,
		listening_socket->ai_family,
		listening_socket->ai_socktype,
		listening_socket->ai_protocol
	);
	if (!ok) return false;

	this->flags.connecting = false;
	this->flags.active = false;
	this->connected_at = this->created_at;	

	established_socket_read_start(this);
	established_socket_write_start(this);

	L_TRACE(L_TRACEID_SOCK_STREAM, TRACE_FMT, TRACE_ARGS);

	return true;
}


bool established_socket_init_connect(struct established_socket * this, struct established_socket_cb * cbs, struct context * context, const struct addrinfo * addr)
{
	int fd = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
	if (fd < 0)
	{
		L_ERROR_ERRNO(errno, "socket(%d, %d, %d)", addr->ai_family, addr->ai_socktype, addr->ai_protocol);
		return false;
	}

	L_TRACE(L_TRACEID_SOCK_STREAM, "fd:%d", fd);

	bool ok = established_socket_init(
		this,
		cbs,
		fd,
		context, 
		addr->ai_addr, addr->ai_addrlen,
		addr->ai_family,
		addr->ai_socktype,
		addr->ai_protocol
	);
	if (!ok)
	{
		return false;
	}

	this->flags.connecting = true;
	this->flags.active = true;

	L_DEBUG("Connecting ...");

	int rc = connect(fd, addr->ai_addr, addr->ai_addrlen);
	if (rc != 0)
	{
		if (errno != EINPROGRESS)
		{
			L_ERROR_ERRNO(errno, "connect()");
			return false;
		}
	}

	established_socket_write_set_event(this, CONNECT_WANT_WRITE);

	L_TRACE(L_TRACEID_SOCK_STREAM, TRACE_FMT, TRACE_ARGS);

	return true;
}

void established_socket_fini(struct established_socket * this)
{
	assert(this != NULL);
	assert(this->read_watcher.fd >= 0);
	assert(this->write_watcher.fd == this->read_watcher.fd);

	L_DEBUG("Socket finalized");

	if(this->cbs->fini != NULL) this->cbs->fini(this);	

	established_socket_read_stop(this);
	established_socket_write_stop(this);

	int rc = close(this->read_watcher.fd);
	if (rc != 0) L_WARN_ERRNO_P(errno, "close()");

	this->read_watcher.fd = -1;
	this->write_watcher.fd = -1;

	this->flags.read_shutdown = true;
	this->flags.write_shutdown = true;
	this->write_shutdown_at = this->read_shutdown_at = ev_now(this->context->ev_loop);
	this->flags.write_open = false;
	this->flags.write_ready = false;

	size_t cap;
	if (this->read_frame != NULL)
	{
		cap = frame_total_start_to_position(this->read_frame);
		frame_pool_return(this->read_frame);
		this->read_frame = NULL;

		if (cap > 0) L_WARN("Lost %zu bytes in read buffer of the socket", cap);
	}

	cap = 0;
	while (this->write_frames != NULL)
	{
		struct frame * frame = this->write_frames;
		this->write_frames = frame->next;

		cap += frame_total_position_to_limit(frame);
		frame_pool_return(frame);
	}
	if (cap > 0) L_WARN("Lost %zu bytes in write buffer of the socket", cap);

	if (this->ssl != NULL)
	{
		SSL_free(this->ssl);
		this->ssl = NULL;
	}
}

///

static void established_socket_error(struct established_socket * this, int syserror, const char * when)
{
	L_WARN_ERRNO(syserror, "System error when %s", when);

	this->syserror = syserror;

	if (this->cbs->error != NULL)
		this->cbs->error(this);

	this->flags.read_shutdown = true;
	this->flags.write_shutdown = true;
	
	this->read_events = 0;
	this->write_events = 0;

	ev_io_stop(this->context->ev_loop, &this->write_watcher);	
	ev_io_stop(this->context->ev_loop, &this->read_watcher);	

	// Perform hard close of the socket
	int rc = shutdown(this->write_watcher.fd, SHUT_RDWR);
	if (rc != 0)
	{
		switch (errno)
		{
			case ENOTCONN:
				break;

			default:
				L_WARN_ERRNO(errno, "shutdown() in error handling");
		}		
	}

	L_TRACE(L_TRACEID_SOCK_STREAM, TRACE_FMT " %s", TRACE_ARGS, when);
}

///

void established_socket_on_connect_event(struct established_socket * this)
{
	assert(this != NULL);
	assert(this->flags.connecting == true);

	L_TRACE(L_TRACEID_SOCK_STREAM, TRACE_FMT, TRACE_ARGS);

	int optval = -1;
	socklen_t optlen = sizeof(optval);
	int rc = getsockopt(this->write_watcher.fd, SOL_SOCKET, SO_ERROR, &optval, &optlen);
	if (rc != 0)
	{
		L_ERROR_ERRNO(errno, "getsockopt(SOL_SOCKET, SO_ERROR) for async connect");
		return;
	}

	if (optval != 0)
	{
		established_socket_error(this, optval, "connecting");
		return;
	}

	L_DEBUG("TCP connection established");
	established_socket_write_unset_event(this, CONNECT_WANT_WRITE);

	if (this->ssl != NULL)
	{
		// Initialize SSL handshake
		established_socket_write_set_event(this, SSL_HANDSHAKE_WANT_WRITE);
		established_socket_read_set_event(this, SSL_HANDSHAKE_WANT_READ);
		
		this->flags.ssl_status = 1;

		L_DEBUG("SSL handshake started");

		// Save one loop
		ev_invoke(this->context->ev_loop, &this->write_watcher, EV_WRITE);

		return;
	}

	// Configure established callbacks
	established_socket_read_start(this);
	established_socket_write_start(this);

	this->connected_at = ev_now(this->context->ev_loop);
	this->flags.connecting = false;
	if (this->cbs->connected != NULL) this->cbs->connected(this);

	// Simulate a write event to dump frames in the write queue
	established_socket_on_write_event(this);

	return;
}

///


void established_socket_read_set_event(struct established_socket * this, enum read_event event)
{
	this->read_events |= event;
	if ((this->read_events != 0) && (this->flags.read_throttle == false) && (this->flags.read_shutdown == false))
		ev_io_start(this->context->ev_loop, &this->read_watcher);
}

void established_socket_read_unset_event(struct established_socket * this, enum read_event event)
{
	this->read_events &= ~event;
	if ((this->read_events == 0) || (this->flags.read_shutdown == true) || (this->flags.read_throttle == true))
		ev_io_stop(this->context->ev_loop, &this->read_watcher);	
}

void established_socket_read_start(struct established_socket * this)
{
	assert(this != NULL);
	assert(this->read_watcher.fd >= 0);

	established_socket_read_set_event(this, READ_WANT_READ);
}


void established_socket_read_stop(struct established_socket * this)
{
	assert(this != NULL);
	assert(this->read_watcher.fd >= 0);

	established_socket_read_unset_event(this, READ_WANT_READ);
}

void established_socket_read_throttle(struct established_socket * this, bool throttle)
{
	assert(this != NULL);
	this->flags.read_throttle = throttle;

	if (throttle) established_socket_read_unset_event(this, 0);
	else established_socket_read_set_event(this, 0);
}


static bool established_socket_uplink_read_stream_end(struct established_socket * this)
{
	struct frame * frame = NULL;
	if ((this->read_frame != NULL) && (frame_total_start_to_position(this->read_frame) == 0))
	{
		// Recycle this->read_frame if there are no data read
		frame = this->read_frame;
		this->read_frame = NULL;

		frame_format_empty(frame);
		frame_set_type(frame, frame_type_STREAM_END);
	}

	if (frame == NULL)
	{
		frame = frame_pool_borrow(&this->context->frame_pool, frame_type_STREAM_END);
	}

	if (frame == NULL)
	{
		L_WARN("Out of frames when preparing end of stream (read)");
		return false;
	}

	bool upstreamed = this->cbs->read(this, frame);
	if (!upstreamed) frame_pool_return(frame);

	return true;
}


static void established_socket_read_shutdown(struct established_socket * this)
{
	L_TRACE(L_TRACEID_SOCK_STREAM, "BEGIN " TRACE_FMT, TRACE_ARGS);

	// Stop futher reads on the socket
	established_socket_read_stop(this);
	this->read_shutdown_at = ev_now(this->context->ev_loop);
	this->flags.read_shutdown = true;

	// Uplink read frame, if there is one (this can result in a partial frame but that's ok)
	if (frame_total_start_to_position(this->read_frame) > 0)
	{
		L_WARN("Partial read due to read shutdown (%zd bytes)", frame_total_start_to_position(this->read_frame));
		bool upstreamed = this->cbs->read(this, this->read_frame);
		if (!upstreamed) frame_pool_return(this->read_frame);
	}
	this->read_frame = NULL;

	// Uplink end-of-stream (reading)
	bool ok = established_socket_uplink_read_stream_end(this);
	if (!ok)
	{
		L_WARN("Failed to submit end-of-stream frame, throttling");
		established_socket_read_throttle(this, true);
		//TODO: Re-enable reading when frames are available again -> this is trottling mechanism
		return;
	}

	L_TRACE(L_TRACEID_SOCK_STREAM, "END " TRACE_FMT, TRACE_ARGS);
}

void established_socket_on_read_event(struct established_socket * this)
{
	bool ok;
	assert(this != NULL);
	assert(this->flags.connecting == false);
	assert(((this->ssl == NULL) && (this->flags.ssl_status == 0)) \
		|| ((this->ssl != NULL) && (this->flags.ssl_status == 2))
	);

	L_TRACE(L_TRACEID_SOCK_STREAM, "BEGIN " TRACE_FMT, TRACE_ARGS);

	this->stats.read_events += 1;

	for (unsigned int read_loops = 0; read_loops<libsccmn_config.sock_est_max_read_loops; read_loops += 1)
	{
		if (this->read_frame == NULL)
		{
			this->read_frame = this->cbs->get_read_frame(this);
			if (this->read_frame == NULL)
			{
				L_WARN("Out of frames when reading, throttling");
				established_socket_read_throttle(this, true);
				//TODO: Re-enable reading when frames are available again -> this is trottling mechanism
				L_TRACE(L_TRACEID_SOCK_STREAM, "END " TRACE_FMT " out of frames", TRACE_ARGS);
				return;
			}
		}

		struct frame_dvec * frame_dvec = frame_current_dvec(this->read_frame);
		assert(frame_dvec != NULL);

		size_t size_to_read = frame_dvec->limit - frame_dvec->position;
		assert(size_to_read > 0);

		void * p_to_read = frame_dvec->frame->data + frame_dvec->offset + frame_dvec->position;

		ssize_t rc;

		if (this->ssl == NULL)
		{
			rc = read(this->read_watcher.fd, p_to_read, size_to_read);

			if (rc <= 0) // Handle error situation
			{
				if (rc < 0)
				{
					if (errno == EAGAIN)
					{
						established_socket_read_set_event(this, READ_WANT_READ);
						L_TRACE(L_TRACEID_SOCK_STREAM, "END " TRACE_FMT " EAGAIN", TRACE_ARGS);
						return;
					}

					established_socket_error(this, errno, "reading");
				}
				else
				{
					L_DEBUG("Peer closed a connection");
					this->syserror = 0;
				}

				established_socket_read_shutdown(this);
				L_TRACE(L_TRACEID_SOCK_STREAM, "END " TRACE_FMT, TRACE_ARGS);
				return;
			}
		}

		else
		{
			try_ssl_read_again:
			rc = SSL_read(this->ssl, p_to_read, size_to_read);
			if (rc <= 0)
			{
				int errno_read = errno;
				int ssl_err = SSL_get_error(this->ssl, rc);
				switch (ssl_err)
				{
					case SSL_ERROR_WANT_READ:
						established_socket_read_set_event(this, READ_WANT_READ);
						established_socket_write_unset_event(this, SSL_READ_WANT_WRITE);
						L_TRACE(L_TRACEID_SOCK_STREAM, "END " TRACE_FMT " READ_WANT_READ", TRACE_ARGS);
						return;

					case SSL_ERROR_WANT_WRITE:
						if (this->flags.write_ready == true)
						{
							this->flags.write_ready = false;
							established_socket_write_set_event(this, WRITE_WANT_WRITE);
							goto try_ssl_read_again;
						}
						established_socket_read_unset_event(this, READ_WANT_READ);
						established_socket_write_set_event(this, SSL_READ_WANT_WRITE);
						L_TRACE(L_TRACEID_SOCK_STREAM, "END " TRACE_FMT " SSL_READ_WANT_WRITE", TRACE_ARGS);
						return;

					case SSL_ERROR_ZERO_RETURN:
						// This is a clean SSL_RECEIVED_SHUTDOWN (read is closed)
						// The socket is still open
						assert(rc == 0); // not sure what to do then rc < -1 ....
						int ssl_shutdown_status = SSL_get_shutdown(this->ssl);
						assert((ssl_shutdown_status & SSL_RECEIVED_SHUTDOWN) == SSL_RECEIVED_SHUTDOWN);
						L_DEBUG("SSL shutdown received");
						this->syserror = 0;
						// Now close also the TCP socket
						rc = shutdown(this->write_watcher.fd, SHUT_RD);
						if ((rc != 0) && (errno != ENOTCONN)) L_WARN_ERRNO_P(errno, "shutdown()");
						established_socket_read_shutdown(this);
						L_TRACE(L_TRACEID_SOCK_STREAM, "END " TRACE_FMT " SSL received shutdown (ssl_shutdown_status: %d)", TRACE_ARGS, ssl_shutdown_status);
						return;

					case SSL_ERROR_SYSCALL:
						if ((rc == 0) && (errno_read == 0))
						{
							// Both SSL and TCP connection has been closed (not via SSL_RECEIVED_SHUTDOWN)
							int ssl_shutdown_status = SSL_get_shutdown(this->ssl);
							assert((ssl_shutdown_status & SSL_RECEIVED_SHUTDOWN) == 0);
							L_DEBUG("Connection closed by peer");
							this->syserror = 0;
							established_socket_read_shutdown(this);
							L_TRACE(L_TRACEID_SOCK_STREAM, "END " TRACE_FMT " SSL terminate (ssl_shutdown_status: %d)", TRACE_ARGS, ssl_shutdown_status);
							return;
						}
						if (errno_read != 0)
						{
							this->syserror = errno_read;
							established_socket_error(this, errno_read, "SSL read (syscall)");
						}
						established_socket_read_shutdown(this);
						L_TRACE(L_TRACEID_SOCK_STREAM, "END " TRACE_FMT " SSL_ERROR_SYSCALL (errno: %d, rc: %zd)", TRACE_ARGS, errno_read, rc);
						return;

					case SSL_ERROR_SSL:
						L_ERROR_OPENSSL("SSL error during read");
						established_socket_error(this, errno_read == 0 ? ECONNRESET : errno_read, "SSL read (SSL error)");
						established_socket_read_shutdown(this);
						L_TRACE(L_TRACEID_SOCK_STREAM, "END " TRACE_FMT " SSL_ERROR_SSL", TRACE_ARGS);
						return;

					default:
						L_ERROR_P("Unexpected error %d  during read, closing", ssl_err);
						established_socket_error(this, errno_read == 0 ? ECONNRESET : errno_read, "SSL read (unknown)");
						established_socket_read_shutdown(this);
						L_TRACE(L_TRACEID_SOCK_STREAM, "END " TRACE_FMT " SSL_ERROR_UNKNOWN", TRACE_ARGS);
						return;

				}
			}
		}

		assert(rc > 0);
		L_TRACE(L_TRACEID_SOCK_STREAM, "READ " TRACE_FMT " rc:%zd", TRACE_ARGS, rc);
		this->stats.read_bytes += rc;
		frame_dvec_position_add(frame_dvec, rc);

		if (frame_dvec->position < frame_dvec->limit)
		{
			// Not all expected data arrived
			if (this->flags.read_partial == true)
			{
				bool upstreamed = this->cbs->read(this, this->read_frame);
				if (upstreamed) this->read_frame = NULL;
			}
			established_socket_read_set_event(this, READ_WANT_READ);
			L_TRACE(L_TRACEID_SOCK_STREAM, "END " TRACE_FMT " incomplete read", TRACE_ARGS);
			return;
		}
		assert(frame_dvec->position == frame_dvec->limit);

		// Current dvec is filled, move to next one
		ok = frame_next_dvec(this->read_frame);
		if (!ok)
		{
			// All dvecs in the frame are filled with data
			bool upstreamed = this->cbs->read(this, this->read_frame);
			if (upstreamed) this->read_frame = NULL;
			if ((this->read_events & READ_WANT_READ) == 0)
			{
				// If watcher is stopped, break reading	
				L_TRACE(L_TRACEID_SOCK_STREAM, "END " TRACE_FMT " read stopped", TRACE_ARGS);
				return;
			}
		}
		else if (this->flags.read_partial == true)
		{
			bool upstreamed = this->cbs->read(this, this->read_frame);
			if (upstreamed) this->read_frame = NULL;
			if ((this->read_events & READ_WANT_READ) == 0)
			{
				// If watcher is stopped, break reading	
				L_TRACE(L_TRACEID_SOCK_STREAM, "END " TRACE_FMT " partial read stopped", TRACE_ARGS);
				return;
			}
		}
	}

	// Maximum read loops within event loop iteration is exceeded
	established_socket_read_set_event(this, READ_WANT_READ);
	L_TRACE(L_TRACEID_SOCK_STREAM, "END " TRACE_FMT " max read loops", TRACE_ARGS);
}

///

void established_socket_write_set_event(struct established_socket * this, enum write_event event)
{
	this->write_events |= event;
	if (this->write_events != 0) ev_io_start(this->context->ev_loop, &this->write_watcher);
}

void established_socket_write_unset_event(struct established_socket * this, enum write_event event)
{
	this->write_events &= ~event;
	if (this->write_events == 0) ev_io_stop(this->context->ev_loop, &this->write_watcher);
}


void established_socket_write_start(struct established_socket * this)
{
	assert(this != NULL);
	assert(this->write_watcher.fd >= 0);

	established_socket_write_set_event(this, WRITE_WANT_WRITE);	
}


void established_socket_write_stop(struct established_socket * this)
{
	assert(this != NULL);
	assert(this->write_watcher.fd >= 0);

	established_socket_write_unset_event(this, WRITE_WANT_WRITE);
}


static void established_socket_write_real(struct established_socket * this)
{
	assert(this != NULL);
	assert(this->flags.write_ready == true);
	assert(this->flags.connecting == false);
	assert(((this->ssl == NULL) && (this->flags.ssl_status == 0)) || ((this->ssl != NULL) && (this->flags.ssl_status == 2)));

	L_TRACE(L_TRACEID_SOCK_STREAM, "BEGIN " TRACE_FMT " Wf:%p", TRACE_ARGS, this->write_frames);	

	while (this->write_frames != NULL)
	{
		L_TRACE(L_TRACEID_SOCK_STREAM, "FRAME " TRACE_FMT " ft:%llx", TRACE_ARGS, this->write_frames->type);

		if (this->write_frames->type == frame_type_STREAM_END)
		{
			struct frame * frame = this->write_frames;
			this->write_frames = frame->next;
			frame_pool_return(frame);

			if (this->write_frames != NULL) L_ERROR("There are data frames in the write queue after end-of-stream.");

			assert(this->flags.write_open == false);
			this->write_shutdown_at = ev_now(this->context->ev_loop);
			this->flags.write_shutdown = true;

			established_socket_write_unset_event(this, WRITE_WANT_WRITE);

			if (this->ssl)
			{
				// Initialize SSL shutdown (writing) by schedule SSL_SENT_SHUTDOWN
				established_socket_on_ssl_sent_shutdown_event(this);
			}

			else
			{
				int rc = shutdown(this->write_watcher.fd, SHUT_WR);
				if (rc != 0) L_ERROR_ERRNO_P(errno, "shutdown()");
			}

			L_TRACE(L_TRACEID_SOCK_STREAM, "END " TRACE_FMT " shutdown", TRACE_ARGS);
			return;
		}

		struct frame_dvec * frame_dvec = frame_current_dvec(this->write_frames);
		assert(frame_dvec != NULL);

		size_t size_to_write = frame_dvec->limit - frame_dvec->position;
		assert(size_to_write > 0);

		const void * p_to_write = frame_dvec->frame->data + frame_dvec->offset + frame_dvec->position;

		ssize_t rc;

		if (this->ssl == NULL)
		{
			rc = write(this->write_watcher.fd, p_to_write, size_to_write);
			if (rc < 0) // Handle error situation
			{
				if (errno == EAGAIN)
				{
					// OS buffer is full, wait for next write event
					this->flags.write_ready = false;
					established_socket_write_set_event(this, WRITE_WANT_WRITE);
					return;
				}

				established_socket_error(this, errno, "writing");
				return;
			}
			else if (rc == 0)
			{
				established_socket_error(this, ECONNRESET, "writing");
				return;
			}
		}

		else
		{
			rc = SSL_write(this->ssl, p_to_write, size_to_write);
			if (rc <= 0)
			{
				int errno_read = errno;
				int ssl_err = SSL_get_error(this->ssl, rc);
				switch (ssl_err)
				{
					case SSL_ERROR_WANT_READ:
						established_socket_read_set_event(this, SSL_WRITE_WANT_READ);
						established_socket_write_unset_event(this, WRITE_WANT_WRITE);
						return;

					case SSL_ERROR_WANT_WRITE:
						this->flags.write_ready = false;
						established_socket_write_set_event(this, WRITE_WANT_WRITE);
						established_socket_read_unset_event(this, SSL_WRITE_WANT_READ);
						return;

					case SSL_ERROR_ZERO_RETURN:
						established_socket_error(this, errno_read == 0 ? ECONNRESET : errno_read, "SSL write (zero ret)");
						return;

					case SSL_ERROR_SYSCALL:
						if ((rc == 0) && (errno_read == 0))
						{
							// This is a quite standard way how SSL connection is closed (by peer)
							L_DEBUG("Peer closed a connection");
							established_socket_write_unset_event(this, WRITE_WANT_WRITE);
							this->syserror = 0;
						}
						else
						{
							established_socket_error(this, errno_read == 0 ? ECONNRESET : errno_read, "SSL write (syscall)");
						}
						return;

					case SSL_ERROR_SSL:
						L_ERROR_OPENSSL("SSL error during write");
						established_socket_error(this, errno_read == 0 ? ECONNRESET : errno_read, "SSL write (SSL error)");
						return;

					default:
						L_WARN_P("Unexpected error %d  during write, closing", ssl_err);
						established_socket_error(this, errno_read == 0 ? ECONNRESET : errno_read, "SSL write (unknown)");
						return;

				}
			}
		}

		assert(rc > 0);
		L_TRACE(L_TRACEID_SOCK_STREAM, "WRITE " TRACE_FMT " rc:%zd", TRACE_ARGS, rc);
		this->stats.write_bytes += rc;
		frame_dvec_position_add(frame_dvec, rc);
		if (frame_dvec->position < frame_dvec->limit)
		{
			// Not all data has been written, wait for next write event
			this->flags.write_ready = false;
			established_socket_write_set_event(this, WRITE_WANT_WRITE);
			return; 
		}
		assert(frame_dvec->position == frame_dvec->limit);

		// Current dvec is filled, move to next one
		bool ok = frame_next_dvec(this->write_frames);
		if (!ok)
		{
			// All dvecs in the frame have been written
			struct frame * frame = this->write_frames;

			this->write_frames = frame->next;
			if (this->write_frames == NULL)
			{
				// There are no more frames to write
				this->write_frame_last = &this->write_frames;
			}

			frame_pool_return(frame);
		}
	}

	return;
}


void established_socket_on_write_event(struct established_socket * this)
{
	assert(this != NULL);
	assert(this->flags.connecting == false);

	L_TRACE(L_TRACEID_SOCK_STREAM, "BEGIN " TRACE_FMT, TRACE_ARGS);

	this->stats.write_events += 1;

	this->flags.write_ready = true;
	established_socket_write_unset_event(this, WRITE_WANT_WRITE);
	if (this->write_frames != NULL)
		established_socket_write_real(this);

	L_TRACE(L_TRACEID_SOCK_STREAM, "END " TRACE_FMT, TRACE_ARGS);
}


bool established_socket_write(struct established_socket * this, struct frame * frame)
{
	assert(this != NULL);

	L_TRACE(L_TRACEID_SOCK_STREAM, "BEGIN " TRACE_FMT, TRACE_ARGS);

	if (this->flags.write_open == false)
	{
		L_WARN_P("Socket is not open for writing");
		return false;
	}

	if (frame->type == frame_type_STREAM_END)
		this->flags.write_open = false;

	//Add frame to the write queue
	*this->write_frame_last = frame;
	frame->next = NULL;
	this->write_frame_last = &frame->next;

	if (this->flags.write_ready == false)
	{
		if (this->flags.connecting == false) established_socket_write_set_event(this, WRITE_WANT_WRITE);
		L_TRACE(L_TRACEID_SOCK_STREAM, "END " TRACE_FMT " queue", TRACE_ARGS);
		return true;
	}

	this->stats.write_direct += 1;

	established_socket_write_real(this);

	L_TRACE(L_TRACEID_SOCK_STREAM, "END " TRACE_FMT " direct", TRACE_ARGS);
	return true;
}

bool established_socket_write_shutdown(struct established_socket * this)
{
	assert(this != NULL);
	
	L_TRACE(L_TRACEID_SOCK_STREAM, "BEGIN " TRACE_FMT, TRACE_ARGS);

	if (this->flags.write_shutdown == true) return true;
	if (this->flags.write_open == false) return true;

	struct frame * frame = frame_pool_borrow(&this->context->frame_pool, frame_type_STREAM_END);
	if (frame == NULL)
	{
		L_WARN("Out of frames when preparing end of stream (write)");
		return false;
	}

	bool ret = established_socket_write(this, frame);
	L_TRACE(L_TRACEID_SOCK_STREAM, "END " TRACE_FMT, TRACE_ARGS);

	return ret;
}

///

bool established_socket_ssl_enable(struct established_socket * this, SSL_CTX *ctx)
{
	assert(this != NULL);
	if (this->ssl != NULL)
	{
		L_WARN("SSL already enabled on the socket");
		return false;
	}
	
	this->ssl = SSL_new(ctx);
	if (this->ssl == NULL)
	{
		L_WARN_OPENSSL("established_socket_ssl_enable:SSL_new");
		return false;
	}

	// Link SSL with socket
	int rc = SSL_set_fd(this->ssl, this->write_watcher.fd);
	if (rc != 1)
	{
		L_WARN_OPENSSL("established_socket_ssl_enable:SSL_set_fd");
		return false;
	}

	return true;
}


void established_socket_on_ssl_handshake_event(struct established_socket * this)
{
	assert(this != NULL);
	assert(this->flags.ssl_status == 1);

	int rc;

	rc = SSL_connect(this->ssl);
	if (rc == 1) // Handshake is  completed
	{
		//TODO: long verify_result = SSL_get_verify_result(seacatcc_context.gwconn_ssl_handle);

		L_DEBUG("SSL connection established");

		// Wire I/O event callbacks
		established_socket_write_unset_event(this, SSL_HANDSHAKE_WANT_WRITE);
		established_socket_read_unset_event(this, SSL_HANDSHAKE_WANT_READ);

		established_socket_read_start(this);
		established_socket_write_start(this);

		this->connected_at = ev_now(this->context->ev_loop);
		this->flags.connecting = false;
		this->flags.ssl_status = 2;
		if (this->cbs->connected != NULL) this->cbs->connected(this);

		// Simulate a write event to dump frames in the write queue
		established_socket_on_write_event(this);

		return;
	}

	int errno_con = errno;
	int ssl_err = SSL_get_error(this->ssl, rc);
	switch (ssl_err)
	{
		case SSL_ERROR_WANT_READ:
			established_socket_read_set_event(this, SSL_HANDSHAKE_WANT_READ);
			established_socket_write_unset_event(this, SSL_HANDSHAKE_WANT_WRITE);
			return;

		case SSL_ERROR_WANT_WRITE:
			established_socket_read_unset_event(this, SSL_HANDSHAKE_WANT_READ);
			established_socket_write_set_event(this, SSL_HANDSHAKE_WANT_WRITE);
			return;

		case SSL_ERROR_ZERO_RETURN:
			established_socket_error(this, errno_con == 0 ? ECONNRESET : errno_con, "SSL connect (zero ret)");
			return;

		case SSL_ERROR_SYSCALL:
			L_WARN_ERRNO(errno_con, "SSL connect (syscall, rc: %d)", rc);
			established_socket_error(this, errno_con == 0 ? ECONNRESET : errno_con, "SSL connect (syscall)");
			return;

		case SSL_ERROR_SSL:
			L_WARN_OPENSSL("SSL error during handshake");
			established_socket_error(this, errno_con == 0 ? ECONNRESET : errno_con, "SSL connect (SSL error)");
			return;

		default:
			L_WARN_P("Unexpected error %d  during handhake, closing", ssl_err);
			established_socket_error(this, errno_con == 0 ? ECONNRESET : errno_con, "SSL connect (unknown)");
			return;

	}

}


void established_socket_on_ssl_sent_shutdown_event(struct established_socket * this)
{
	// This function handles outgoing SSL shutdown (SSL_SENT_SHUTDOWN)

	assert(this != NULL);
	assert(this->flags.ssl_status != 0);
	assert(this->ssl != NULL);

	L_TRACE(L_TRACEID_SOCK_STREAM, "BEGIN " TRACE_FMT, TRACE_ARGS);


	int ssl_shutdown_status = SSL_get_shutdown(this->ssl);
	if ((ssl_shutdown_status & SSL_SENT_SHUTDOWN) == SSL_SENT_SHUTDOWN)
	{
		L_WARN("SSL shutdown has been already sent");
		L_TRACE(L_TRACEID_SOCK_STREAM, "END " TRACE_FMT " already sent", TRACE_ARGS);
		return;
	}

	int rc = SSL_shutdown(this->ssl);

	if (rc == 0)
	{
		//SSL_SENT_SHUTDOWN has been sent
		L_DEBUG("SSL shutdown sent");
		established_socket_write_unset_event(this, SSL_SHUTDOWN_WANT_WRITE);
		established_socket_read_unset_event(this, SSL_SHUTDOWN_WANT_READ);
		L_TRACE(L_TRACEID_SOCK_STREAM, "END " TRACE_FMT " sent", TRACE_ARGS);
		return;
	}

	if (rc == 1) // SSL Shutdown is fully completed (peer already closed connection as well)
	{
		this->flags.ssl_status = 0;

		established_socket_write_unset_event(this, SSL_SHUTDOWN_WANT_WRITE);
		established_socket_read_unset_event(this, SSL_SHUTDOWN_WANT_READ);

		L_DEBUG("SSL connection has been shutdown");

		int rc = shutdown(this->write_watcher.fd, SHUT_RDWR);
		if (rc != 0) L_WARN_ERRNO_P(errno, "shutdown()");

		L_TRACE(L_TRACEID_SOCK_STREAM, "END " TRACE_FMT " completed", TRACE_ARGS);
		return;
	}

	int errno_ssl_cmd = errno;
	int ssl_err = SSL_get_error(this->ssl, rc);
	switch (ssl_err)
	{
		case SSL_ERROR_WANT_READ:
			established_socket_read_set_event(this, SSL_SHUTDOWN_WANT_READ);
			established_socket_write_unset_event(this, SSL_SHUTDOWN_WANT_WRITE);
			L_TRACE(L_TRACEID_SOCK_STREAM, "END " TRACE_FMT " SSL_SHUTDOWN_WANT_READ", TRACE_ARGS);
			return;

		case SSL_ERROR_WANT_WRITE:
			established_socket_read_unset_event(this, SSL_SHUTDOWN_WANT_READ);
			established_socket_write_set_event(this, SSL_SHUTDOWN_WANT_WRITE);
			L_TRACE(L_TRACEID_SOCK_STREAM, "END " TRACE_FMT " SSL_SHUTDOWN_WANT_WRITE", TRACE_ARGS);
			return;

		case SSL_ERROR_ZERO_RETURN:
		case SSL_ERROR_SYSCALL:
		case SSL_ERROR_SSL:
		default:
			// What to do here ...
			L_FATAL_P("Unexpected error %d  during SSL shutdown, closing", ssl_err);
			established_socket_error(this, errno_ssl_cmd == 0 ? ECONNRESET : errno_ssl_cmd, "SSL shutdown (unknown)");
			L_TRACE(L_TRACEID_SOCK_STREAM, "END " TRACE_FMT " unknown", TRACE_ARGS);
			return;
	}

}

///

static void established_socket_on_read(struct ev_loop * loop, struct ev_io * watcher, int revents)
{
	struct established_socket * this = watcher->data;
	assert(this != NULL);

	L_TRACE(L_TRACEID_SOCK_STREAM, "BEGIN " TRACE_FMT " e:%x ei:%u", TRACE_ARGS, revents, ev_iteration(loop));

	if ((revents & EV_ERROR) != 0)
	{
		established_socket_error(this, ECONNRESET, "reading (libev)");
		goto end;
	}

	if ((revents & EV_READ) == 0) goto end;

	if (this->flags.read_throttle)
	{
		ev_io_stop(this->context->ev_loop, &this->read_watcher);
		goto end;
	}

	if (this->read_events & READ_WANT_READ)
		established_socket_on_read_event(this);

	if (this->ssl != NULL)
	{
		if (this->read_events & SSL_HANDSHAKE_WANT_READ)
		{
			established_socket_on_ssl_handshake_event(this);
		}

		if (this->read_events & SSL_SHUTDOWN_WANT_READ)
		{
			established_socket_on_ssl_sent_shutdown_event(this);
		}

		if (this->read_events & SSL_WRITE_WANT_READ)
		{
			established_socket_on_write_event(this);
		}
	}

end:
	L_TRACE(L_TRACEID_SOCK_STREAM, "END " TRACE_FMT " e:%x ei:%u", TRACE_ARGS, revents, ev_iteration(loop));
}


static void established_socket_on_write(struct ev_loop * loop, struct ev_io * watcher, int revents)
{
	struct established_socket * this = watcher->data;
	assert(this != NULL);

	L_TRACE(L_TRACEID_SOCK_STREAM, "BEGIN " TRACE_FMT " e:%x ei:%u", TRACE_ARGS, revents, ev_iteration(loop));

	if ((revents & EV_ERROR) != 0)
	{
		established_socket_error(this, ECONNRESET, "writing (libev)");
		goto end;
	}

	if ((revents & EV_WRITE) == 0) goto end;

	if (this->write_events & CONNECT_WANT_WRITE)
		established_socket_on_connect_event(this);

	if (this->write_events & WRITE_WANT_WRITE)
		established_socket_on_write_event(this);

	if (this->ssl != NULL)
	{
		if (this->write_events & SSL_HANDSHAKE_WANT_WRITE)
		{
			established_socket_on_ssl_handshake_event(this);
		}

		if (this->write_events & SSL_SHUTDOWN_WANT_WRITE)
		{
			established_socket_on_ssl_sent_shutdown_event(this);
		}

		if (this->write_events & SSL_READ_WANT_WRITE)
		{
			established_socket_on_read_event(this);
		}
	}

end:
	L_TRACE(L_TRACEID_SOCK_STREAM, "END " TRACE_FMT " e:%x ei:%u", TRACE_ARGS, revents, ev_iteration(loop));
}

///

struct frame * get_read_frame_simple(struct established_socket * this)
{
	struct frame * frame = frame_pool_borrow(&this->context->frame_pool, frame_type_RAW_DATA);
	frame_format_simple(frame);
	return frame;
}
