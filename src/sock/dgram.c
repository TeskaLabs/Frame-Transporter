#include "../_ft_internal.h"

// TODO: Conceptual: Consider implementing readv() and writev() versions for more complicated frame formats (eh, on linux it is implemented as memcpy)
// TODO: SO_KEEPALIVE
// TODO: SO_LINGER (in a non-blocking mode?, valid for SEQPACKET?)
// TODO: SO_PRIORITY
// TODO: SO_RCVBUF
// TODO: SO_SNDBUF (??)
// TODO: SO_RCVLOWAT (consider using)
// TODO: SO_RCVTIMEO & SO_SNDTIMEO

const char * ft_dgram_class = "ft_dgram";

static void _ft_dgram_on_read(struct ev_loop * loop, struct ev_io * watcher, int revents);
static void _ft_dgram_on_write(struct ev_loop * loop, struct ev_io * watcher, int revents);

enum _ft_dgram_read_event
{
	READ_WANT_READ = 1,
};

enum _ft_dgram_write_event
{
	WRITE_WANT_WRITE = 1,
};

static void _ft_dgram_read_set_event(struct ft_dgram * this, enum _ft_dgram_read_event event);
static void _ft_dgram_read_unset_event(struct ft_dgram * this, enum _ft_dgram_read_event event);
static void _ft_dgram_write_set_event(struct ft_dgram * this, enum _ft_dgram_write_event event);
static void _ft_dgram_write_unset_event(struct ft_dgram * this, enum _ft_dgram_write_event event);

static void _ft_dgram_on_write_event(struct ft_dgram * this);

///

#define TRACE_FMT "fd:%d B:%c C:%c S:%c Rt:%c Re:%c Rw:%c Wo:%c Wr:%c We:%c Ww:%c E:(%d)"
#define TRACE_ARGS \
	this->read_watcher.fd, \
	(this->flags.bind) ? 'Y' : '.', \
	(this->flags.connect) ? 'Y' : '.', \
	(this->flags.shutdown) ? 'Y' : '.', \
	(this->flags.read_throttle) ? 'Y' : '.', \
	(this->read_events & READ_WANT_READ) ? 'R' : '.', \
	(ev_is_active(&this->read_watcher)) ? 'Y' : '.', \
	(this->flags.write_open) ? 'Y' : '.', \
	(this->flags.write_ready) ? 'Y' : '.', \
	(this->write_events & WRITE_WANT_WRITE) ? 'W' : '.', \
	(ev_is_active(&this->write_watcher)) ? 'Y' : '.', \
	this->error.sys_errno

///

bool ft_dgram_init(struct ft_dgram * this, struct ft_dgram_delegate * delegate, struct ft_context * context, int family, int socktype, int protocol)
{
	bool ok;
	int i, rc;

	assert(this != NULL);
	assert(delegate != NULL);
	assert(socktype == SOCK_DGRAM);

	FT_TRACE(FT_TRACE_ID_DGRAM, "BEGIN");

	ok = ft_socket_init_(
		&this->base.socket, ft_dgram_class, context,
		family, socktype, protocol,
		NULL, 0
	);
	if (!ok) return false;	

	int fd = socket(family, socktype, protocol);
	if (fd < 0)
	{
		FT_ERROR_ERRNO(errno, "socket(%d, %d, %d)", family, socktype, protocol);
		return false;
	}

	FT_TRACE(FT_TRACE_ID_DGRAM, "SOCKET fd:%d", fd);

	// Set OS buffer size, it is important to maintain a capability to send larger frame in a single datagram
	i = FRAME_SIZE;
	rc = setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &i, sizeof(i));
	if (rc == -1) FT_WARN_ERRNO(errno, "setsockopt(%d, SOL_SOCKET, SO_SNDBUF, %d)", fd, i);

	i = FRAME_SIZE;
	rc = setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &i, sizeof(i));
	if (rc == -1) FT_WARN_ERRNO(errno, "setsockopt(%d, SOL_SOCKET, SO_RCVBUF, %d)", fd, i);

	ok = ft_fd_nonblock(fd);
	if (!ok) FT_WARN_ERRNO(errno, "Failed when setting established socket to non-blocking mode");

	this->delegate = delegate;
	this->read_frame = NULL;
	this->write_frames = NULL;
	this->write_frame_last = &this->write_frames;
	this->stats.read_events = 0;
	this->stats.write_events = 0;
	this->stats.write_direct = 0;
	this->stats.read_bytes = 0;
	this->stats.write_bytes = 0;
	this->flags.read_throttle = false;	
	this->flags.write_open = true;
	this->flags.write_open = true;
	this->flags.shutdown = false;
	this->flags.bind = false;
	this->flags.connect = false;

	this->error.sys_errno = 0;

	assert(this->base.socket.context->ev_loop != NULL);
	this->created_at = ev_now(this->base.socket.context->ev_loop);
	this->shutdown_at = NAN;

	ev_io_init(&this->read_watcher, _ft_dgram_on_read, fd, EV_READ);
	ev_set_priority(&this->read_watcher, -1); // Read has always lower priority than writing
	this->read_watcher.data = this;
	this->read_events = 0;

	ev_io_init(&this->write_watcher, _ft_dgram_on_write, fd, EV_WRITE);
	ev_set_priority(&this->write_watcher, 1);
	this->write_watcher.data = this;
	this->write_events = 0;
	this->flags.write_ready = false;

	ok = ft_dgram_cntl(this, FT_DGRAM_WRITE_START);
	if (!ok) FT_WARN_P("Failed to set events properly");

	FT_TRACE(FT_TRACE_ID_DGRAM, "END " TRACE_FMT, TRACE_ARGS);

	return true;
}

void ft_dgram_fini(struct ft_dgram * this)
{
	assert(this != NULL);
	assert(this->base.socket.clazz == ft_dgram_class);

	assert(this->read_watcher.fd >= 0);
	assert(this->write_watcher.fd == this->read_watcher.fd);

	if(this->delegate->fini != NULL) this->delegate->fini(this);	

	ft_dgram_cntl(this, FT_DGRAM_READ_STOP | FT_DGRAM_WRITE_STOP);

	int rc = close(this->read_watcher.fd);
	if (rc != 0) FT_WARN_ERRNO_P(errno, "close()");

	if ((this->base.socket.ai_family == AF_UNIX) && (this->base.socket.addrlen > 0))
	{
		struct sockaddr_un * un = (struct sockaddr_un *)&this->base.socket.addr;
		
		rc = unlink(un->sun_path);
		if (rc != 0) FT_WARN_ERRNO(errno, "Unlinking unix socket '%s'", un->sun_path);
	}

	this->read_watcher.fd = -1;
	this->write_watcher.fd = -1;

	if (this->flags.shutdown == false)
	{
		this->flags.shutdown = true;
		this->shutdown_at = ev_now(this->base.socket.context->ev_loop);
	}

	this->flags.write_open = false;
	this->flags.write_ready = false;

	size_t cap;
	if (this->read_frame != NULL)
	{
		cap = ft_frame_pos(this->read_frame);
		ft_frame_return(this->read_frame);
		this->read_frame = NULL;

		if (cap > 0) FT_WARN("Lost %zu bytes in read buffer of the socket", cap);
	}

	cap = ft_dgram_trash_write_buffer(this);
	if (cap > 0) FT_WARN("Lost %zu bytes in write buffer of the socket", cap);
}

size_t ft_dgram_trash_write_buffer(struct ft_dgram * this)
{
	assert(this != NULL);

	size_t cap = 0;
	while (this->write_frames != NULL)
	{
		struct ft_frame * frame = this->write_frames;
		this->write_frames = frame->next;

		cap += ft_frame_len(frame);
		ft_frame_return(frame);
	}
	return cap;
}

///

bool ft_dgram_bind(struct ft_dgram * this, const struct sockaddr * addr, socklen_t addrlen)
{
	bool ok;
	int rc;

	assert(this != NULL);
	assert(this->base.socket.clazz == ft_dgram_class);
	assert(addr != NULL);
	assert(this->read_watcher.fd >= 0);

	FT_TRACE(FT_TRACE_ID_DGRAM, "BEGIN " TRACE_FMT, TRACE_ARGS);

	if (this->flags.bind == true) FT_WARN("Datagram socket is bound already");

	if ((this->base.socket.ai_family == AF_UNIX) && (addrlen > 0))
	{
		// Remove stalled unix socket
		struct sockaddr_un * un = (struct sockaddr_un *)addr;

		struct stat statbuf;
		rc = stat(un->sun_path, &statbuf);
		if ((rc == 0) && (S_ISSOCK(statbuf.st_mode)))
		{
			rc = unlink(un->sun_path);
			if (rc != 0) FT_WARN_ERRNO(errno, "unlink(%s)", un->sun_path);
			else FT_WARN("Stalled unix datagram socket '%s', deleting", un->sun_path);
		}
	}

	rc = bind(this->read_watcher.fd, addr, addrlen);
	if (rc != 0)
	{
		if (errno != EINPROGRESS)
		{
			FT_ERROR_ERRNO(errno, "bind of datagram socket");
			FT_TRACE(FT_TRACE_ID_DGRAM, "END bind err " TRACE_FMT, TRACE_ARGS);
			return false;
		}
	}

	this->base.socket.addrlen = addrlen;
	memcpy(&this->base.socket.addr, addr, addrlen);

	this->flags.bind = true;

	ok = ft_dgram_cntl(this, FT_DGRAM_READ_START);
	if (!ok) FT_WARN_P("Failed to set events properly");

	FT_TRACE(FT_TRACE_ID_DGRAM, "END " TRACE_FMT, TRACE_ARGS);

	return true;
}

bool ft_dgram_connect(struct ft_dgram * this, const struct sockaddr * addr, socklen_t addrlen)
{
	bool ok;
	int rc;

	assert(this != NULL);
	assert(this->base.socket.clazz == ft_dgram_class);
	assert(addr != NULL);
	assert(this->read_watcher.fd >= 0);

	FT_TRACE(FT_TRACE_ID_DGRAM, "BEGIN " TRACE_FMT, TRACE_ARGS);

	if (this->flags.connect == true) FT_WARN("Datagram socket is connected already");

	rc = connect(this->read_watcher.fd, addr, addrlen);
	if (rc != 0)
	{
		if (errno != EINPROGRESS)
		{
			FT_ERROR_ERRNO(errno, "connect()");
			FT_TRACE(FT_TRACE_ID_DGRAM, "END connect err" TRACE_FMT, TRACE_ARGS);
			return false;
		}
	}

	this->flags.connect = true;

	ok = ft_dgram_cntl(this, FT_DGRAM_WRITE_START);
	if (!ok) FT_WARN_P("Failed to set events properly");

	FT_TRACE(FT_TRACE_ID_DGRAM, "END " TRACE_FMT, TRACE_ARGS);

	return true;	
}

///

static void _ft_dgram_shutdown_real(struct ft_dgram * this, bool uplink_eos)
{
	FT_TRACE(FT_TRACE_ID_DGRAM, "BEGIN " TRACE_FMT, TRACE_ARGS);

	// Stop futher reads on the socket
	ft_dgram_cntl(this, FT_DGRAM_READ_STOP | FT_DGRAM_READ_STOP);

	this->shutdown_at = ev_now(this->base.socket.context->ev_loop);
	this->flags.shutdown = true;
	this->flags.write_open = false;

	int rc = shutdown(this->write_watcher.fd, SHUT_RDWR);
	if (rc != 0)
	{
		if (errno == ENOTCONN) { /* NO-OP ... this can happen when connection is closed quickly after connecting */ }
		else
			FT_WARN_ERRNO_P(errno, "shutdown()");
	}

	// Uplink (to delegate) end-of-stream
	if (uplink_eos)
	{
		struct ft_frame * frame = NULL;
		if ((this->read_frame != NULL) && (ft_frame_pos(this->read_frame) == 0))
		{
			// Recycle this->read_frame if there are no data read
			frame = this->read_frame;
			this->read_frame = NULL;

			ft_frame_format_empty(frame);
			ft_frame_set_type(frame, FT_FRAME_TYPE_END_OF_STREAM);
		}

		if (frame == NULL)
		{
			frame = ft_pool_borrow(&this->base.socket.context->frame_pool, FT_FRAME_TYPE_END_OF_STREAM);
		}

		if (frame == NULL)
		{
			//TODO: call this->delegate->error() to indicate out-of-frames situation
			FT_TRACE(FT_TRACE_ID_DGRAM, "END " TRACE_FMT " out of frames", TRACE_ARGS);
			return;
		}

		assert(this->delegate != NULL);
		assert(this->delegate->read != NULL);
		bool upstreamed = this->delegate->read(this, frame);
		if (!upstreamed) ft_frame_return(frame);
	}

	FT_TRACE(FT_TRACE_ID_DGRAM, "END " TRACE_FMT, TRACE_ARGS);
}

static void _ft_dgram_error(struct ft_dgram * this, int sys_errno, const char * when)
{
	assert(sys_errno != 0);

	FT_TRACE(FT_TRACE_ID_DGRAM, "BEGIN " TRACE_FMT " %s", TRACE_ARGS, when);
	FT_ERROR_ERRNO(sys_errno, "System error on datagram socket when %s", when);

	this->error.sys_errno = sys_errno;

	if (this->delegate->error != NULL)
		this->delegate->error(this);

	_ft_dgram_shutdown_real(this, false);

	FT_TRACE(FT_TRACE_ID_DGRAM, "END " TRACE_FMT " %s", TRACE_ARGS, when);
}

///

void _ft_dgram_read_set_event(struct ft_dgram * this, enum _ft_dgram_read_event event)
{
	assert(this != NULL);
	assert(this->base.socket.clazz == ft_dgram_class);

	this->read_events |= event;
	if ((this->read_events != 0) && (this->flags.read_throttle == false) && (this->flags.shutdown == false))
		ev_io_start(this->base.socket.context->ev_loop, &this->read_watcher);
}

void _ft_dgram_read_unset_event(struct ft_dgram * this, enum _ft_dgram_read_event event)
{
	assert(this != NULL);
	assert(this->base.socket.clazz == ft_dgram_class);

	this->read_events &= ~event;
	if ((this->read_events == 0) || (this->flags.shutdown == true) || (this->flags.read_throttle == true))
		ev_io_stop(this->base.socket.context->ev_loop, &this->read_watcher);	
}

bool _ft_dgram_cntl_read_start(struct ft_dgram * this)
{
	assert(this != NULL);
	assert(this->base.socket.clazz == ft_dgram_class);

	assert(this->read_watcher.fd >= 0);

	_ft_dgram_read_set_event(this, READ_WANT_READ);
	return true;
}


bool _ft_dgram_cntl_read_stop(struct ft_dgram * this)
{
	assert(this != NULL);
	assert(this->base.socket.clazz == ft_dgram_class);

	assert(this->read_watcher.fd >= 0);

	_ft_dgram_read_unset_event(this, READ_WANT_READ);
	return true;
}

bool _ft_dgram_cntl_read_throttle(struct ft_dgram * this, bool throttle)
{
	assert(this != NULL);
	assert(this->base.socket.clazz == ft_dgram_class);

	this->flags.read_throttle = throttle;

	if (throttle) _ft_dgram_read_unset_event(this, 0);
	else _ft_dgram_read_set_event(this, 0);

	return true;
}


void _ft_dgram_on_read_event(struct ft_dgram * this)
{
	ssize_t rc;

	assert(this != NULL);
	assert(this->base.socket.clazz == ft_dgram_class);

	FT_TRACE(FT_TRACE_ID_DGRAM, "BEGIN " TRACE_FMT, TRACE_ARGS);

	this->stats.read_events += 1;

	for (unsigned int read_loops = 0; read_loops<ft_config.sock_est_max_read_loops; read_loops += 1)
	{
		if (this->read_frame == NULL)
		{
			if (this->delegate->get_read_frame != NULL)
			{
				this->read_frame = this->delegate->get_read_frame(this);
			}
			else
			{
				this->read_frame = ft_pool_borrow(&this->base.socket.context->frame_pool, FT_FRAME_TYPE_RAW_DATA);
				if (this->read_frame != NULL) ft_frame_format_simple(this->read_frame);
			}

			if (this->read_frame == NULL)
			{
				FT_WARN("Out of frames when reading, throttling");
				ft_dgram_cntl(this, FT_DGRAM_READ_PAUSE);
				//TODO: Re-enable reading when frames are available again -> this is trottling mechanism
				FT_TRACE(FT_TRACE_ID_DGRAM, "END " TRACE_FMT " out of frames", TRACE_ARGS);
				return;
			}
		}

		struct ft_vec * frame_dvec = ft_frame_get_vec(this->read_frame);
		assert(frame_dvec != NULL);

		//TODO: Transform frame_dvec into iovec array (aka support multiple frame_vec)
		struct iovec iov[1] = {
			{
				.iov_base = frame_dvec->frame->data + frame_dvec->offset + frame_dvec->position,
				.iov_len = frame_dvec->limit - frame_dvec->position,
			}
		};
		assert(iov[0].iov_len > 0);

		struct msghdr msghdr = {
			.msg_name = &this->read_frame->addr,
			.msg_namelen = sizeof(this->read_frame->addr),
			.msg_iov = iov,
			.msg_iovlen = 1,
			.msg_control = NULL,
			.msg_controllen = 0,
			.msg_flags = 0
		};

		rc = recvmsg(this->read_watcher.fd, &msghdr, 0);
		if (rc <= 0) // Handle error situation
		{
			if (rc < 0)
			{
				if (errno == EAGAIN)
				{
					_ft_dgram_read_set_event(this, READ_WANT_READ);
					FT_TRACE(FT_TRACE_ID_DGRAM, "END " TRACE_FMT " EAGAIN", TRACE_ARGS);
					return;
				}

				_ft_dgram_error(this, errno, "reading");
			}
			else
			{
				FT_WARN_P("Recv on DGRAM returned 0 - why?");
				
				this->error.sys_errno = 0;
				_ft_dgram_shutdown_real(this, true);
			}

			FT_TRACE(FT_TRACE_ID_DGRAM, "END recvmsg:%zd " TRACE_FMT, rc, TRACE_ARGS);
			return;
		}

		assert(rc > 0);
		FT_TRACE(FT_TRACE_ID_DGRAM, "READ " TRACE_FMT " rc:%zd", TRACE_ARGS, rc);
		this->read_frame->addrlen = msghdr.msg_namelen;
		this->stats.read_bytes += rc;
		ft_vec_advance(frame_dvec, rc);

		// Current dvec is filled, move to next one
		bool ok = ft_frame_next_vec(this->read_frame);
		if (!ok)
		{
			// All dvecs in the frame are filled with data
			bool upstreamed = this->delegate->read(this, this->read_frame);
			if (upstreamed) this->read_frame = NULL;
			if ((this->read_events & READ_WANT_READ) == 0)
			{
				// If watcher is stopped, break reading	
				FT_TRACE(FT_TRACE_ID_DGRAM, "END " TRACE_FMT " read stopped", TRACE_ARGS);
				return;
			}
		}
	}

	// Maximum read loops within event loop iteration is exceeded
	_ft_dgram_read_set_event(this, READ_WANT_READ);
	FT_TRACE(FT_TRACE_ID_DGRAM, "END " TRACE_FMT " max read loops", TRACE_ARGS);
}

///

void _ft_dgram_write_set_event(struct ft_dgram * this, enum _ft_dgram_write_event event)
{
	assert(this != NULL);
	assert(this->base.socket.clazz == ft_dgram_class);

	this->write_events |= event;
	if (this->write_events != 0) ev_io_start(this->base.socket.context->ev_loop, &this->write_watcher);
}

void _ft_dgram_write_unset_event(struct ft_dgram * this, enum _ft_dgram_write_event event)
{
	assert(this != NULL);
	assert(this->base.socket.clazz == ft_dgram_class);

	this->write_events &= ~event;
	if (this->write_events == 0) ev_io_stop(this->base.socket.context->ev_loop, &this->write_watcher);
}


bool _ft_dgram_cntl_write_start(struct ft_dgram * this)
{
	assert(this != NULL);
	assert(this->base.socket.clazz == ft_dgram_class);

	assert(this->write_watcher.fd >= 0);

	_ft_dgram_write_set_event(this, WRITE_WANT_WRITE);
	return true;
}


bool _ft_dgram_cntl_write_stop(struct ft_dgram * this)
{
	assert(this != NULL);
	assert(this->base.socket.clazz == ft_dgram_class);

	assert(this->write_watcher.fd >= 0);

	_ft_dgram_write_unset_event(this, WRITE_WANT_WRITE);
	return true;
}


static void _ft_dgram_return_write_frame(struct ft_dgram * this)
{
	struct ft_frame * frame = this->write_frames;

	this->write_frames = frame->next;
	if (this->write_frames == NULL)
	{
		// There are no more frames to write
		this->write_frame_last = &this->write_frames;
	}

	ft_frame_return(frame);
}

static void _ft_dgram_write_real(struct ft_dgram * this)
{
	ssize_t rc;

	assert(this != NULL);
	assert(this->base.socket.clazz == ft_dgram_class);

	assert(this->flags.write_ready == true);

	FT_TRACE(FT_TRACE_ID_DGRAM, "BEGIN " TRACE_FMT " Wf:%p", TRACE_ARGS, this->write_frames);	

	unsigned int write_loop = 0;
	while (this->write_frames != NULL)
	{
		FT_TRACE(FT_TRACE_ID_DGRAM, "FRAME " TRACE_FMT " ft:%llx", TRACE_ARGS, (unsigned long long)this->write_frames->type);

		if (write_loop > ft_config.sock_est_max_read_loops)
		{
			// Maximum write loops per event loop iteration reached
			this->flags.write_ready = false;
			_ft_dgram_write_set_event(this, WRITE_WANT_WRITE);
			FT_TRACE(FT_TRACE_ID_DGRAM, "END " TRACE_FMT " max loops", TRACE_ARGS);
			return; 
		}
		write_loop += 1;

		if (this->write_frames->type == FT_FRAME_TYPE_END_OF_STREAM)
		{
			struct ft_frame * frame = this->write_frames;
			this->write_frames = frame->next;
			ft_frame_return(frame);

			if (this->write_frames != NULL) FT_ERROR("There are data frames in the write queue after end-of-stream.");

			_ft_dgram_shutdown_real(this, true);

			FT_TRACE(FT_TRACE_ID_DGRAM, "END " TRACE_FMT " shutdown", TRACE_ARGS);
			return;
		}

		struct ft_vec * frame_dvec = ft_frame_get_vec(this->write_frames);
		assert(frame_dvec != NULL);

		//TODO: Transform frame_dvec into iovec array (aka support multiple frame_vec) - but this has to be configurable
		struct iovec iov[1] = {
			{
				.iov_base = frame_dvec->frame->data + frame_dvec->offset + frame_dvec->position,
				.iov_len = frame_dvec->limit - frame_dvec->position,
			}
		};
		assert(iov[0].iov_len > 0);

		struct msghdr msghdr = {
			.msg_iov = iov,
			.msg_iovlen = 1,
			.msg_control = NULL,
			.msg_controllen = 0,
			.msg_flags = 0
		};

		// If requested, prepare a message control block from a last vector in the frame
		if (this->write_frames->flags.msg_control == true)
		{
			struct ft_vec * vec = ft_frame_get_vec_at(this->write_frames, -1);
			if (vec == NULL)
			{
				FT_WARN("Frame has msg_control flag set but no vector with msg_control");
			}
			else
			{
				// Remove the last vector
				this->write_frames->vec_limit -= 1;

				msghdr.msg_control = ft_vec_ptr(vec);
				msghdr.msg_controllen = vec->limit;
			}
		}

		// If socket is connected, then a frame address will be ignored
		if (this->flags.connect)
		{
			msghdr.msg_name = NULL;
			msghdr.msg_namelen = 0;
		}
		else
		{
			msghdr.msg_name = &this->write_frames->addr;
			msghdr.msg_namelen = this->write_frames->addrlen;
		}

		FT_TRACE(FT_TRACE_ID_DGRAM, "sendmsg PRE size:%zd nl:%d " TRACE_FMT, iov[0].iov_len, msghdr.msg_namelen, TRACE_ARGS);

		rc = sendmsg(this->write_watcher.fd, &msghdr, 0);
		if (rc < 0) // Handle error situation
		{
			int lerrno = errno;
			if ((lerrno == EAGAIN) || (lerrno == ENOBUFS))
			{
				// OS buffer is full, wait for next write event
				this->flags.write_ready = false;
				_ft_dgram_write_set_event(this, WRITE_WANT_WRITE);
				FT_TRACE(FT_TRACE_ID_DGRAM, "END " TRACE_FMT " EAGAIN", TRACE_ARGS);
				return;
			}

			// If socket is not connected, don't close that when the system error is observed
			if (this->flags.connect)
			{
				_ft_dgram_error(this, lerrno, "writing");
			}
			else
			{
				FT_WARN_ERRNO(lerrno, "System error on datagram socket when writting, the frame is lost");
				_ft_dgram_return_write_frame(this);
			}

			FT_TRACE(FT_TRACE_ID_DGRAM, "END " TRACE_FMT " write errno:%d", TRACE_ARGS, lerrno);
			return;
		}
		else if (rc == 0)
		{
			//TODO: Test if this is a best possible reaction
			FT_WARN("Zero write occured, will retry soon");
			this->flags.write_ready = false;
			_ft_dgram_write_set_event(this, WRITE_WANT_WRITE);

			FT_TRACE(FT_TRACE_ID_DGRAM, "END " TRACE_FMT " zero-write", TRACE_ARGS);
			return;
		}

		assert(rc > 0);
		FT_TRACE(FT_TRACE_ID_DGRAM, "sendmsg POST rc:%zd " TRACE_FMT, rc, TRACE_ARGS);
		this->stats.write_bytes += rc;
		ft_vec_advance(frame_dvec, rc);
		if (frame_dvec->position < frame_dvec->limit)
		{
			// Not all data has been written, wait for next write event
			this->flags.write_ready = false;
			_ft_dgram_write_set_event(this, WRITE_WANT_WRITE);
			FT_TRACE(FT_TRACE_ID_DGRAM, "END " TRACE_FMT " partial", TRACE_ARGS);
			return; 
		}
		assert(frame_dvec->position == frame_dvec->limit);

		// Current dvec is filled, move to next one
		bool ok = ft_frame_next_vec(this->write_frames);
		if (!ok)
		{
			// All dvecs in the frame have been written
			_ft_dgram_return_write_frame(this);
		}
	}

	FT_TRACE(FT_TRACE_ID_DGRAM, "END " TRACE_FMT, TRACE_ARGS);
	return;
}


void _ft_dgram_on_write_event(struct ft_dgram * this)
{
	assert(this != NULL);
	assert(this->base.socket.clazz == ft_dgram_class);

	FT_TRACE(FT_TRACE_ID_DGRAM, "BEGIN " TRACE_FMT, TRACE_ARGS);

	this->stats.write_events += 1;
	this->flags.write_ready = true;
	
	if (this->write_frames != NULL)
	{
		this->write_events &= ~WRITE_WANT_WRITE; // Just pretend that we are stopping a watcher
		_ft_dgram_write_real(this);
		if (this->write_events == 0) // Now do it for real if needed
			_ft_dgram_write_unset_event(this, WRITE_WANT_WRITE);
	}
	else
	{
		_ft_dgram_write_unset_event(this, WRITE_WANT_WRITE);
	}

	FT_TRACE(FT_TRACE_ID_DGRAM, "END " TRACE_FMT, TRACE_ARGS);
}


bool ft_dgram_write(struct ft_dgram * this, struct ft_frame * frame)
{
	assert(this != NULL);
	assert(this->base.socket.clazz == ft_dgram_class);

	FT_TRACE(FT_TRACE_ID_DGRAM, "BEGIN " TRACE_FMT, TRACE_ARGS);

	if (this->flags.write_open == false)
	{
		if (frame->type == FT_FRAME_TYPE_END_OF_STREAM)
		{
			ft_frame_return(frame);
			return true;
		}
		FT_WARN("Datagram socket is not open for writing (f:%p ft: %08llx)", frame, (unsigned long long) frame->type);
		return false;
	}

	if (frame->type == FT_FRAME_TYPE_END_OF_STREAM)
		this->flags.write_open = false;

	//Add frame to the write queue
	*this->write_frame_last = frame;
	frame->next = NULL;
	this->write_frame_last = &frame->next;

	if (this->flags.write_ready == false)
	{
		_ft_dgram_write_set_event(this, WRITE_WANT_WRITE);
		FT_TRACE(FT_TRACE_ID_DGRAM, "END " TRACE_FMT " queue", TRACE_ARGS);
		return true;
	}

	this->stats.write_direct += 1;

	_ft_dgram_write_real(this);

	FT_TRACE(FT_TRACE_ID_DGRAM, "END " TRACE_FMT " direct", TRACE_ARGS);
	return true;
}

bool _ft_dgram_cntl_shutdown(struct ft_dgram * this)
{
	assert(this != NULL);
	assert(this->base.socket.clazz == ft_dgram_class);
	
	FT_TRACE(FT_TRACE_ID_DGRAM, "BEGIN " TRACE_FMT, TRACE_ARGS);

	if (this->flags.shutdown == true) return true;
	if (this->flags.write_open == false) return true;

	struct ft_frame * frame = ft_pool_borrow(&this->base.socket.context->frame_pool, FT_FRAME_TYPE_END_OF_STREAM);
	if (frame == NULL)
	{
		FT_WARN("Out of frames when preparing end of stream (write)");
		return false;
	}

	bool ret = ft_dgram_write(this, frame);
	FT_TRACE(FT_TRACE_ID_DGRAM, "END " TRACE_FMT, TRACE_ARGS);

	return ret;
}

///

static void _ft_dgram_on_read(struct ev_loop * loop, struct ev_io * watcher, int revents)
{
	struct ft_dgram * this = watcher->data;

	assert(this != NULL);
	assert(this->base.socket.clazz == ft_dgram_class);

	FT_TRACE(FT_TRACE_ID_DGRAM, "BEGIN " TRACE_FMT " e:%x ei:%u", TRACE_ARGS, revents, ev_iteration(loop));

	if ((revents & EV_ERROR) != 0)
	{
		_ft_dgram_error(this, ECONNRESET, "reading (libev)");
		goto end;
	}

	if ((revents & EV_READ) == 0) goto end;

	if (this->flags.read_throttle)
	{
		ev_io_stop(this->base.socket.context->ev_loop, &this->read_watcher);
		goto end;
	}

	if (this->read_events & READ_WANT_READ)
		_ft_dgram_on_read_event(this);

end:
	FT_TRACE(FT_TRACE_ID_DGRAM, "END " TRACE_FMT " e:%x ei:%u", TRACE_ARGS, revents, ev_iteration(loop));
}


static void _ft_dgram_on_write(struct ev_loop * loop, struct ev_io * watcher, int revents)
{
	struct ft_dgram * this = watcher->data;

	assert(this != NULL);
	assert(this->base.socket.clazz == ft_dgram_class);

	FT_TRACE(FT_TRACE_ID_DGRAM, "BEGIN " TRACE_FMT " e:%x ei:%u", TRACE_ARGS, revents, ev_iteration(loop));

	if ((revents & EV_ERROR) != 0)
	{
		_ft_dgram_error(this, ECONNRESET, "writing (libev)");
		goto end;
	}

	if ((revents & EV_WRITE) == 0) goto end;

	if (this->write_events & WRITE_WANT_WRITE)
		_ft_dgram_on_write_event(this);

end:
	FT_TRACE(FT_TRACE_ID_DGRAM, "END " TRACE_FMT " e:%x ei:%u", TRACE_ARGS, revents, ev_iteration(loop));
}

void ft_dgram_diagnose(struct ft_dgram * this)
{
	assert(this != NULL);
	assert(this->base.socket.clazz == ft_dgram_class);

	fprintf(stderr, TRACE_FMT "\n", TRACE_ARGS);
}

