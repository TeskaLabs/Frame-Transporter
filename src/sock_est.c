#include "all.h"

static void established_socket_on_read(struct ev_loop * loop, struct ev_io * watcher, int revents);

///

bool established_socket_init(struct established_socket * this, struct established_socket_cb * cbs, struct frame_pool * frame_pool, struct ev_loop * loop, struct listening_socket * listening_socket, int fd, const struct sockaddr * peer_addr, socklen_t peer_addr_len)
{
	assert(frame_pool != NULL);
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
	this->frame_pool = frame_pool;
	this->read_frame = NULL;

	this->establish_at = ev_now(loop);
	this->shutdown_at = NAN;
	this->close_at = NAN;

	memcpy(&this->ai_addr, peer_addr, peer_addr_len);
	this->ai_addrlen = peer_addr_len;

	this->ai_family = listening_socket->ai_family;
	this->ai_socktype = listening_socket->ai_socktype;
	this->ai_protocol = listening_socket->ai_protocol;

	ev_io_init(&this->read_watcher, established_socket_on_read, fd, EV_READ);
	this->read_watcher.data = this;

	ev_io_init(&this->write_watcher, NULL, fd, EV_WRITE);
	this->write_watcher.data = this;

	return true;
}

// It is static for a reason
// The close operation should be triggered by a graceful established_socket_shutdown with a timeout operation
static void established_socket_close(struct established_socket * this, struct ev_loop * loop)
{
	assert(this->read_watcher.fd >= 0);
	assert(this->write_watcher.fd == this->read_watcher.fd);

	established_socket_read_stop(loop, this);

	assert(this->cbs->close != NULL);
	this->cbs->close(this, loop);

	int rc = close(this->read_watcher.fd);
	if (rc != 0) L_WARN_ERRNO_P(errno, "close()");

	this->read_watcher.fd = -1;
	this->write_watcher.fd = -1;

	this->close_at = ev_now(loop);

	if (this->read_frame != NULL)
	{
		size_t cap = frame_total_position(this->read_frame);
		frame_pool_return(this->read_frame);
		this->read_frame = NULL;

		if (cap > 0) L_WARN("Lost %zu bytes in read buffer of the socket", cap);
	}
}

void established_socket_fini(struct established_socket * this, struct ev_loop * loop)
{
	if (this->read_watcher.fd >= 0)
	{
		established_socket_close(this, loop);
	}
}


bool established_socket_read_start(struct ev_loop * loop, struct established_socket * this)
{
	assert(loop != NULL);
	assert(this != NULL);

	if (this->read_watcher.fd < 0)
	{
		L_WARN("Reading on socket that is not open!");
		return false;
	}

	ev_io_start(loop, &this->read_watcher);
	return true;
}


bool established_socket_read_stop(struct ev_loop * loop, struct established_socket * this)
{
	assert(loop != NULL);
	assert(this != NULL);

	if (this->read_watcher.fd < 0)
	{
		L_WARN("Reading (stop) on socket that is not open!");
		return false;
	}

	ev_io_stop(loop, &this->read_watcher);
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

	if (revents & EV_READ)
	{
		if (this->read_frame == NULL)
		{
			this->read_frame = frame_pool_borrow(this->frame_pool);
			if (this->read_frame == NULL)
			{
				L_WARN("Out of frames when reading, reading stopped");
				established_socket_read_stop(loop, this);
				//TODO: Re-enable reading when frames are available again
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
			established_socket_close(this, loop);
			return;
		}

		else if (rc == 0)
		{
			established_socket_close(this, loop);
			return;		
		}

		frame_dvec_position_add(frame_dvec, rc);
		bool upstreamed = this->cbs->read(this, loop, this->read_frame);
		if (upstreamed) this->read_frame = NULL;
	}
}

bool established_socket_shutdown(struct ev_loop * loop, struct established_socket * this)
{
	assert(this != NULL);
	
	if (this->write_watcher.fd < 0)
	{
		L_WARN("Calling shutdown on closed socket");
		return false;
	}

	ev_io_stop(loop, &this->write_watcher);

	int rc = shutdown(this->write_watcher.fd, SHUT_WR);
	if (rc != 0)
	{
		L_ERROR_ERRNO_P(errno, "shutdown()");
		return false;
	}

	this->shutdown_at = ev_now(loop);

	return true;
}

