#include "all.h"

static void established_socket_on_read(struct ev_loop * loop, struct ev_io * watcher, int revents);
static void established_socket_on_write(struct ev_loop * loop, struct ev_io * watcher, int revents);

///

bool established_socket_init_accept(struct established_socket * this, struct established_socket_cb * cbs, struct listening_socket * listening_socket, int fd, const struct sockaddr * peer_addr, socklen_t peer_addr_len)
{
	assert(this != NULL);
	assert(cbs != NULL);
	assert(listening_socket != NULL);

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
	this->context = listening_socket->context;
	this->read_frame = NULL;

	this->write_frames = NULL;
	this->write_frame_last = &this->write_frames;

	assert(this->context->ev_loop != NULL);
	this->establish_at = ev_now(this->context->ev_loop);

	this->shutdown_at = NAN;
	this->close_at = NAN;

	memcpy(&this->ai_addr, peer_addr, peer_addr_len);
	this->ai_addrlen = peer_addr_len;

	this->ai_family = listening_socket->ai_family;
	this->ai_socktype = listening_socket->ai_socktype;
	this->ai_protocol = listening_socket->ai_protocol;

	ev_io_init(&this->read_watcher, established_socket_on_read, fd, EV_READ);
	this->read_watcher.data = this;

	ev_io_init(&this->write_watcher, established_socket_on_write, fd, EV_WRITE);
	this->write_watcher.data = this;
	this->writeable = false;

	established_socket_write_start(this);

	return true;
}

// It is static for a reason
// The close operation should be triggered by a graceful established_socket_shutdown with a timeout operation
static void established_socket_close(struct established_socket * this)
{
	assert(this->read_watcher.fd >= 0);
	assert(this->write_watcher.fd == this->read_watcher.fd);

	established_socket_read_stop(this);

	assert(this->cbs->close != NULL);
	this->cbs->close(this);

	int rc = close(this->read_watcher.fd);
	if (rc != 0) L_WARN_ERRNO_P(errno, "close()");

	this->read_watcher.fd = -1;
	this->write_watcher.fd = -1;

	this->close_at = ev_now(this->context->ev_loop);

	if (this->read_frame != NULL)
	{
		size_t cap = frame_total_position(this->read_frame);
		frame_pool_return(this->read_frame);
		this->read_frame = NULL;

		if (cap > 0) L_WARN("Lost %zu bytes in read buffer of the socket", cap);
	}
}

void established_socket_fini(struct established_socket * this)
{
	//TODO: Return write frames ...

	if (this->read_watcher.fd >= 0)
	{
		established_socket_close(this);
	}
}


bool established_socket_read_start(struct established_socket * this)
{
	assert(this != NULL);

	if (this->read_watcher.fd < 0)
	{
		L_WARN("Reading on socket that is not open!");
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


void established_socket_on_read(struct ev_loop * loop, struct ev_io * watcher, int revents)
{
	struct established_socket * this = watcher->data;
	assert(this != NULL);

	if (revents & EV_ERROR)
	{
		L_ERROR("Established socket (read) got error event");
		return;
	}

	if ((revents & EV_READ) == 0) return;

	if (this->read_frame == NULL)
	{
		this->read_frame = frame_pool_borrow(&this->context->frame_pool);
		if (this->read_frame == NULL)
		{
			L_WARN("Out of frames when reading, reading stopped");
			established_socket_read_stop(this);
			//TODO: Re-enable reading when frames are available again -> this is trottling mechanism
			return;
		}

		frame_format_simple(this->read_frame);
	}

	size_t size_to_read = this->cbs->read_advise(this, this->read_frame);
	struct frame_dvec * frame_dvec = &this->read_frame->dvecs[0];

	ssize_t rc = read(watcher->fd, frame_dvec->frame->data + frame_dvec->position, size_to_read);
	if (rc < 0)
	{
		L_ERROR_ERRNO(errno, "Reading for peer");
		established_socket_close(this);
		return;
	}

	else if (rc == 0)
	{
		established_socket_close(this);
		return;		
	}

	frame_dvec_position_add(frame_dvec, rc);
	bool upstreamed = this->cbs->read(this, this->read_frame);
	if (upstreamed) this->read_frame = NULL;
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
	assert(this->writeable == true);

	while (this->write_frames != NULL)
	{
		//TODO: Improve to support frames with more than one dvec
		struct frame_dvec * frame_dvec = &this->write_frames->dvecs[0];

		ssize_t rc = write(this->write_watcher.fd, frame_dvec->frame->data + frame_dvec->position, frame_dvec->limit - frame_dvec->position);
		printf("write RC: %zd\n", rc);

		if (rc < 0)
		{
			//ERROR !!!
			L_ERROR_ERRNO(errno, "established_socket_write_real!");

			return false;
		}

		frame_dvec->position += rc;
		assert(frame_dvec->position <= frame_dvec->limit);
		if (frame_dvec->position == frame_dvec->limit)
		{
			// Frame has been written completely, iterate to next one

			this->write_frames = this->write_frames->next;
			if (this->write_frames == NULL)
			{
				// There are no more frames to write
				this->write_frame_last = &this->write_frames;
			}
		}

		else
		{
			L_WARN_P("Partial write");
			return true; //We likely exceeded a size of OS output buffer, so now wait for next write event
		}
	}

	return false;
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

	this->writeable = true;

	bool start = established_socket_write_real(this);
	if (!start) ev_io_stop(this->context->ev_loop, &this->write_watcher);
}


void established_socket_write(struct established_socket * this, struct frame * frame)
{
	assert(this != NULL);

	//Add frame to the write queue
	*this->write_frame_last = frame;
	frame->next = NULL;
	this->write_frame_last = &frame->next;

	if (this->writeable == false)
	{
		ev_io_start(this->context->ev_loop, &this->write_watcher);
		//TODO: Increment statistic about this 
		return;
	}

	bool start = established_socket_write_real(this);
	if (start) ev_io_start(this->context->ev_loop, &this->write_watcher);
}

///

bool established_socket_shutdown(struct established_socket * this)
{
	assert(this != NULL);
	
	if (this->write_watcher.fd < 0)
	{
		L_WARN("Calling shutdown on closed socket");
		return false;
	}

	ev_io_stop(this->context->ev_loop, &this->write_watcher);

	int rc = shutdown(this->write_watcher.fd, SHUT_WR);
	if (rc != 0)
	{
		L_ERROR_ERRNO_P(errno, "shutdown()");
		return false;
	}

	this->shutdown_at = ev_now(this->context->ev_loop);
	//TODO: Close socket after some time ...

	return true;
}

