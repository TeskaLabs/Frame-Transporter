#include "all.h"

// TODO: Conceptual: Consider implementing readv() and writev() versions for more complicated frame formats

static void established_socket_on_read(struct ev_loop * loop, struct ev_io * watcher, int revents);
static void established_socket_on_write(struct ev_loop * loop, struct ev_io * watcher, int revents);
static inline void established_socket_state_changed(struct established_socket *);

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
	this->stats.direct_write_events = 0;
	this->stats.read_bytes = 0;
	this->stats.write_bytes = 0;
	this->flags.read_partial = false;
	this->flags.read_shutdown = false;
	this->read_shutdown_at = NAN;
	this->flags.write_shutdown = false;
	this->write_shutdown_at = NAN;
	this->flags.write_open = true;

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

	ev_io_init(&this->write_watcher, established_socket_on_write, fd, EV_WRITE);
	ev_set_priority(&this->write_watcher, 1);
	this->write_watcher.data = this;
	this->flags.write_ready = false;

	return true;
}

bool established_socket_init_accept(struct established_socket * this, struct established_socket_cb * cbs, struct listening_socket * listening_socket, int fd, const struct sockaddr * peer_addr, socklen_t peer_addr_len)
{
	assert(listening_socket != NULL);

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

	established_socket_write_start(this);
	established_socket_state_changed(this);

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

	L_DEBUG("Connecting to ...");

	int rc = connect(fd, addr->ai_addr, addr->ai_addrlen);
	if (rc != 0)
	{
		if (errno != EINPROGRESS)
		{
			L_ERROR_ERRNO(errno, "connect()");
			return false;
		}
	}

	established_socket_write_start(this);
	established_socket_state_changed(this);

	return true;
}

void established_socket_fini(struct established_socket * this)
{
	assert(this != NULL);
	assert(this->read_watcher.fd >= 0);
	assert(this->write_watcher.fd == this->read_watcher.fd);

	L_DEBUG("Socket finalized");

	if(this->cbs->close != NULL) this->cbs->close(this);	

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
}

///

void established_socket_state_changed(struct established_socket * this)
{
	if(this->cbs->state_changed != NULL) this->cbs->state_changed(this);
}

static void established_socket_error(struct established_socket * this, int syserror)
{
	L_DEBUG_ERRNO(syserror, "Socket error");

	this->syserror = syserror;

	if (this->cbs->error != NULL)
		this->cbs->error(this);

	this->flags.read_shutdown = true;
	this->flags.write_shutdown = true;
	established_socket_write_stop(this);
	established_socket_read_stop(this);

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
}

///

bool established_socket_read_start(struct established_socket * this)
{
	assert(this != NULL);

	if (this->read_watcher.fd < 0)
	{
		L_WARN("Requesting reading on the socket that is not open!");
		return false;
	}

	if (this->flags.read_shutdown == true)
	{
		L_WARN("Requesting reading on the socket that has been shut down");
		return false;
	}

	ev_io_start(this->context->ev_loop, &this->read_watcher);
	return true;
}


bool established_socket_read_stop(struct established_socket * this)
{
	assert(this != NULL);

	if (this->read_watcher.fd < 0)
	{
		L_WARN("Reading (stop) on socket that is not open!");
		return false;
	}

	ev_io_stop(this->context->ev_loop, &this->read_watcher);
	return true;
}


static bool established_socket_uplink_read_stream_end(struct established_socket * this)
{
	//TODO: Recycle this->read_frame if there are no data
	struct frame * frame = frame_pool_borrow(&this->context->frame_pool, frame_type_STREAM_END);
	if (frame == NULL)
	{
		L_WARN("Out of frames when preparing end of stream (read)");
		return false;
	}

	frame_format_simple(frame);

	bool upstreamed = this->cbs->read(this, frame);
	if (!upstreamed) frame_pool_return(frame);

	return true;
}


void established_socket_on_read(struct ev_loop * loop, struct ev_io * watcher, int revents)
{
	bool ok;
	struct established_socket * this = watcher->data;
	assert(this != NULL);

	this->stats.read_events += 1;

	if (revents & EV_ERROR)
	{
		L_ERROR("Established socket (read) got error event");
		return;
	}

	if ((revents & EV_READ) == 0) return;

	for (;;)
	{
		if (this->read_frame == NULL)
		{
			this->read_frame = this->cbs->get_read_frame(this);
			if (this->read_frame == NULL)
			{
				L_WARN("Out of frames when reading, reading stopped");
				established_socket_read_stop(this);
				//TODO: Re-enable reading when frames are available again -> this is trottling mechanism
				return;
			}
		}

		struct frame_dvec * frame_dvec = frame_current_dvec(this->read_frame);
		assert(frame_dvec != NULL);

		size_t size_to_read = frame_dvec->limit - frame_dvec->position;
		assert(size_to_read > 0);

		ssize_t rc = read(watcher->fd, frame_dvec->frame->data + frame_dvec->offset + frame_dvec->position, size_to_read);

		if (rc <= 0) // Handle error situation
		{
			if (rc < 0)
			{
				if (errno == EAGAIN) return;

				established_socket_error(this, errno);
				this->syserror = errno;
				return;
			}
			else
			{
				this->syserror = 0;
			}

			// Stop futher reads on the socket
			established_socket_read_stop(this);
			this->read_shutdown_at = ev_now(this->context->ev_loop);
			this->flags.read_shutdown = true;

			//TODO: Uplink read frame, if there is one

			// Uplink end-of-stream (reading)
			ok = established_socket_uplink_read_stream_end(this);
			if (!ok)
			{
				L_WARN("Failed to submit end-of-stream frame");
				//TODO: Re-enable reading when frames are available again -> this is trottling mechanism
				return;
			}

			established_socket_state_changed(this);		

			return;		
		}

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
			if (!ev_is_active(&this->read_watcher)) return; // If watcher is stopped, break reading
		}
		else if (this->flags.read_partial == true)
		{
			bool upstreamed = this->cbs->read(this, this->read_frame);
			if (upstreamed) this->read_frame = NULL;
			if (!ev_is_active(&this->read_watcher)) return; // If watcher is stopped, break reading		
		}
	}
}

///

bool established_socket_write_start(struct established_socket * this)
{
	assert(this != NULL);

	if (this->read_watcher.fd < 0)
	{
		L_WARN("Writing on socket that is not open!");
		return false;
	}

	ev_io_start(this->context->ev_loop, &this->write_watcher);
	return true;
}


bool established_socket_write_stop(struct established_socket * this)
{
	assert(this != NULL);

	if (this->read_watcher.fd < 0)
	{
		L_WARN("Writing (stop) on socket that is not open!");
		return false;
	}

	ev_io_stop(this->context->ev_loop, &this->write_watcher);
	return true;
}


static bool established_socket_write_real(struct established_socket * this)
// Returns true if write watcher should be started
// Returns false if write watcher should be stopped
{
	assert(this != NULL);
	assert(this->flags.write_ready == true);

	this->stats.write_events += 1;

	while (this->write_frames != NULL)
	{
		if (this->write_frames->type == frame_type_STREAM_END)
		{
			struct frame * frame = this->write_frames;
			this->write_frames = frame->next;
			frame_pool_return(frame);

			if (this->write_frames != NULL) L_ERROR("There are data frames in the write queue after end-of-stream.");

			assert(this->flags.write_open == false);
			this->write_shutdown_at = ev_now(this->context->ev_loop);
			this->flags.write_shutdown = true;

			int rc = shutdown(this->write_watcher.fd, SHUT_WR);
			if (rc != 0) L_ERROR_ERRNO_P(errno, "shutdown()");

			return false;
		}

		struct frame_dvec * frame_dvec = frame_current_dvec(this->write_frames);
		assert(frame_dvec != NULL);

		size_t size_to_write = frame_dvec->limit - frame_dvec->position;
		assert(size_to_write > 0);

		ssize_t rc = write(this->write_watcher.fd, frame_dvec->frame->data + frame_dvec->offset + frame_dvec->position, size_to_write);

		if (rc < 0) // Handle error situation
		{
			if (errno == EAGAIN)
			{
				this->flags.write_ready = false;
				return true; // OS buffer is full, wait for next write event
			}

			established_socket_error(this, errno);
			this->syserror = errno;
			return false;
		}

		this->stats.write_bytes += rc;
		frame_dvec_position_add(frame_dvec, rc);
		if (frame_dvec->position < frame_dvec->limit)
		{
			// Not all data has been written, wait for next write event
			this->flags.write_ready = false;
			return true; 
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

	return false;
}


static bool established_socket_on_connected(struct established_socket * this, struct ev_loop * loop, int fd)
{
	int optval = -1;
	socklen_t optlen = sizeof (optval);

	int rc = getsockopt(fd, SOL_SOCKET, SO_ERROR, &optval, &optlen);
	if (rc != 0)
	{
		L_ERROR_ERRNO(errno, "getsockopt(SOL_SOCKET, SO_ERROR) for async connect");
		return false;
	}

	if (optval != 0)
	{
		L_WARN_ERRNO(optval,"Error when connecting a socket");
		established_socket_error(this, optval);
		return false;
	}

	this->connected_at = ev_now(loop);
	this->flags.connecting = false;
	this->cbs->connected(this);

	L_DEBUG("Connected");

	return true;
}


void established_socket_on_write(struct ev_loop * loop, struct ev_io * watcher, int revents)
{
	struct established_socket * this = watcher->data;
	assert(this != NULL);

	if (revents & EV_ERROR)
	{
		L_ERROR("Established socket (write) got error event");
		return;
	}

	if ((revents & EV_WRITE) == 0) return;

	if (this->flags.connecting == true)
	{
		bool ok = established_socket_on_connected(this, loop, watcher->fd);
		if (!ok) return;
	}

	this->flags.write_ready = true;

	bool start = established_socket_write_real(this);
	if (!start) ev_io_stop(this->context->ev_loop, &this->write_watcher);
}


bool established_socket_write(struct established_socket * this, struct frame * frame)
{
	assert(this != NULL);

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
		ev_io_start(this->context->ev_loop, &this->write_watcher);
		return true;
	}

	this->stats.direct_write_events += 1;

	bool start = established_socket_write_real(this);
	if (start) ev_io_start(this->context->ev_loop, &this->write_watcher);

	return true;
}

///

bool established_socket_shutdown(struct established_socket * this)
{
	assert(this != NULL);

	// When .connecting, do shutdown directly ...
	if (this->flags.connecting)
	{
		int rc = shutdown(this->write_watcher.fd, SHUT_RDWR);
		if (rc != 0)
		{
			switch (errno)
			{
				case ENOTCONN:
					break;

				default:
					L_WARN_ERRNO(errno, "shutdown() in error handling");
					return false;
			}		
		}
		return true;
	}
	
	if (this->flags.write_shutdown == true) return true;
	if (this->flags.write_open == false) return true;

	struct frame * frame = frame_pool_borrow(&this->context->frame_pool, frame_type_STREAM_END);
	if (frame == NULL)
	{
		L_WARN("Out of frames when preparing end of stream (read)");
		return false;
	}
	frame_format_simple(frame);

	return established_socket_write(this, frame);
}

///

struct frame * get_read_frame_simple(struct established_socket * this)
{
	struct frame * frame = frame_pool_borrow(&this->context->frame_pool, frame_type_RAW_DATA);
	frame_format_simple(frame);
	return frame;
}
