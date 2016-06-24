#include "all.h"

static void established_socket_on_read(struct ev_loop * loop, struct ev_io * watcher, int revents);
static void established_socket_on_write(struct ev_loop * loop, struct ev_io * watcher, int revents);
static inline void established_socket_state_changed(struct established_socket *);

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
	this->read_advise = 0;
	this->read_size = 0;
	this->write_frames = NULL;
	this->write_frame_last = &this->write_frames;
	this->stats.read_events = 0;
	this->stats.write_events = 0;
	this->stats.direct_write_events = 0;
	this->stats.read_bytes = 0;
	this->stats.write_bytes = 0;

	this->flags.read_connected = true;
	this->flags.write_connected = true;
	this->flags.write_open = true;

	assert(this->context->ev_loop != NULL);
	this->established_at = ev_now(this->context->ev_loop);
	this->read_shutdown_at = NAN;

	memcpy(&this->ai_addr, peer_addr, peer_addr_len);
	this->ai_addrlen = peer_addr_len;

	this->ai_family = listening_socket->ai_family;
	this->ai_socktype = listening_socket->ai_socktype;
	this->ai_protocol = listening_socket->ai_protocol;

	ev_io_init(&this->read_watcher, established_socket_on_read, fd, EV_READ);
	ev_set_priority(&this->read_watcher, -1); // Read has always lower priority than writing
	this->read_watcher.data = this;

	ev_io_init(&this->write_watcher, established_socket_on_write, fd, EV_WRITE);
	ev_set_priority(&this->write_watcher, 1);
	this->write_watcher.data = this;
	this->flags.write_ready = false;

	established_socket_write_start(this);
	established_socket_state_changed(this);

	return true;
}


void established_socket_fini(struct established_socket * this)
{
	assert(this != NULL);
	assert(this->read_watcher.fd >= 0);
	assert(this->write_watcher.fd == this->read_watcher.fd);

	if(this->cbs->close != NULL) this->cbs->close(this);	

	established_socket_read_stop(this);
	established_socket_write_stop(this);

	int rc = close(this->read_watcher.fd);
	if (rc != 0) L_WARN_ERRNO_P(errno, "close()");

	this->read_watcher.fd = -1;
	this->write_watcher.fd = -1;

	this->flags.read_connected = false;
	this->flags.write_connected = false;
	this->flags.write_open = false;
	this->flags.write_ready = false;

	size_t cap;
	if (this->read_frame != NULL)
	{
		cap = frame_total_position(this->read_frame);
		frame_pool_return(this->read_frame);
		this->read_frame = NULL;

		if (cap > 0) L_WARN("Lost %zu bytes in read buffer of the socket", cap);
	}

	cap = 0;
	while (this->write_frames != NULL)
	{
		struct frame * frame = this->write_frames;
		this->write_frames = frame->next;

		cap += frame_total_limit(frame);
		frame_pool_return(frame);
	}
	if (cap > 0) L_WARN("Lost %zu bytes in write buffer of the socket", cap);
}

///

void established_socket_state_changed(struct established_socket * this)
{
	if(this->cbs->state_changed != NULL) this->cbs->state_changed(this);
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

	if (this->flags.read_connected == false)
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

	if (this->read_frame == NULL)
	{
		this->read_frame = frame_pool_borrow(&this->context->frame_pool, frame_type_RAW_DATA);
		if (this->read_frame == NULL)
		{
			L_WARN("Out of frames when reading, reading stopped");
			established_socket_read_stop(this);
			//TODO: Re-enable reading when frames are available again -> this is trottling mechanism
			return;
		}

		frame_format_simple(this->read_frame);

		this->read_advise = this->cbs->read_advise(this, this->read_frame);
		this->read_size = 0;
	}

	size_t size_to_read;
	if (this->read_advise == 0)
	{
		// Read as much as possible
		size_to_read = frame_currect_dvec_size(this->read_frame);
	} else {
		size_to_read = this->read_advise - this->read_size;
	}

	struct frame_dvec * frame_dvec = frame_current_dvec(this->read_frame);
	assert(frame_dvec != NULL);

	//TODO: IMPORTANT: Adjust size_to_read to fit that into a current dvec
	ssize_t rc = read(watcher->fd, frame_dvec->frame->data + frame_dvec->position, size_to_read);

	if (rc <= 0)
	{
		// Handle error situation
		if (rc < 0)
		{
			if (errno == EAGAIN) return;
			this->read_syserror = errno;
			L_ERROR_ERRNO_P(errno, "read()");
		}
		else
		{
			this->read_syserror = 0;
		}

		// Stop futher reads on the socket
		established_socket_read_stop(this);

		//TODO: Uplink read frame, if there is one

		// Uplink end-of-stream (reading)
		ok = established_socket_uplink_read_stream_end(this);
		if (!ok)
		{
			L_WARN("Failed to submit end-of-stream frame");
			//TODO: Re-enable reading when frames are available again -> this is trottling mechanism
			return;
		}

		this->read_shutdown_at = ev_now(this->context->ev_loop);
		this->flags.read_connected = false;
		established_socket_state_changed(this);		

		return;		
	}

	this->stats.read_bytes += rc;

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
			this->flags.write_connected = false;

			int rc = shutdown(this->write_watcher.fd, SHUT_WR);
			if (rc != 0) L_ERROR_ERRNO_P(errno, "shutdown()");

			return false;
		}

		struct frame_dvec * frame_dvec = frame_current_dvec(this->write_frames);

		assert(frame_dvec->limit - frame_dvec->position > 0);
		ssize_t rc = write(this->write_watcher.fd, frame_dvec->frame->data + frame_dvec->position, frame_dvec->limit - frame_dvec->position);

		if (rc < 0)
		{
			if (errno == EAGAIN)
			{
				this->flags.write_ready = false;
				return true; // OS buffer is full, wait for next write event
			}

			L_ERROR_ERRNO_P(errno, "write(%zd)", frame_dvec->limit - frame_dvec->position);
			return false;
		}

		this->stats.write_bytes += rc;

		frame_dvec->position += rc;
		assert(frame_dvec->position <= frame_dvec->limit);
		if (frame_dvec->position == frame_dvec->limit)
		{
			// Frame has been written completely, iterate to next one

			struct frame * frame = this->write_frames;

			this->write_frames = frame->next;
			if (this->write_frames == NULL)
			{
				// There are no more frames to write
				this->write_frame_last = &this->write_frames;
			}

			frame_pool_return(frame);
		}

		else
		{
			//We likely exceeded a size of OS output buffer, so now wait for next write event
			this->flags.write_ready = false;
			return true; 
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
	
	if (this->flags.write_connected == false) return true;

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

size_t established_socket_read_advise_max_frame(struct established_socket * this, struct frame * frame)
{
	return frame_currect_dvec_size(frame);
}

size_t established_socket_read_advise_opportunistic(struct established_socket * this, struct frame * frame)
{
	return 0;
}
