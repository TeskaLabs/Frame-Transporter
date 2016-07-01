#ifndef __LIBSCCMN_SOCK_EST_H__
#define __LIBSCCMN_SOCK_EST_H__

struct established_socket;

struct established_socket_cb
{
	struct frame * (*get_read_frame)(struct established_socket *);

	// True as a return value means, that the frame has been handed over to upstream protocol
	bool (*read)(struct established_socket *, struct frame * frame);

	void (*state_changed)(struct established_socket *); // Can be NULL

	void (*close)(struct established_socket *);

	void (*connected)(struct established_socket *); // Called when connect() is successfully established; can be NULL

	void (*error)(struct established_socket *);
};

struct established_socket
{
	// Common fields
	struct context * context;

	struct
	{
		unsigned int read_shutdown : 1;  // Socket is read-wise connected
		unsigned int write_shutdown : 1; // Socket is write-wise connected
		unsigned int write_open : 1;      // Write queue is open for adding new frames
		unsigned int write_ready : 1;     // We can write to the socket (no need to wait for EV_WRITE)
		unsigned int connecting : 1;
		unsigned int active : 1;
		unsigned int read_partial : 1; // When yes, read() callback is triggered for any incoming data
		unsigned int ssl_status : 2; // 0 - disconnected; 1 - in handshake; 2 - established; 3 - closing
	} flags;

	int ai_family;
	int ai_socktype;
	int ai_protocol;

	struct sockaddr_storage ai_addr;
	socklen_t ai_addrlen;

	struct established_socket_cb * cbs;

	ev_tstamp created_at;
	ev_tstamp connected_at;
	ev_tstamp read_shutdown_at;
	ev_tstamp write_shutdown_at;

	int syserror;

	// Input
	struct ev_io read_watcher;
	struct frame * read_frame;

	// Output
	struct ev_io write_watcher;
	struct frame * write_frames; // Queue of write frames 
	struct frame ** write_frame_last;

	// SSL
	SSL *ssl;

	// Statistics
	struct
	{
		unsigned int read_events;
		unsigned int write_events;
		unsigned int direct_write_events; //Writes without need of wait for EV_WRITE
		unsigned long read_bytes;
		unsigned long write_bytes;
	} stats;

	// Custom data field
	void * data;
};

bool established_socket_init_accept(struct established_socket *, struct established_socket_cb * cbs, struct listening_socket * listening_socket, int fd, const struct sockaddr * peer_addr, socklen_t peer_addr_len);
bool established_socket_init_connect(struct established_socket *, struct established_socket_cb * cbs, struct context * context, const struct addrinfo * addr);
void established_socket_fini(struct established_socket *);

bool established_socket_read_start(struct established_socket *);
bool established_socket_read_stop(struct established_socket *);

bool established_socket_write_start(struct established_socket *);
bool established_socket_write_stop(struct established_socket *);

bool established_socket_write(struct established_socket *, struct frame * frame);
bool established_socket_write_shutdown(struct established_socket *);

static inline bool established_socket_is_shutdown(struct established_socket * this)
{
	return (this->flags.write_shutdown && this->flags.write_shutdown);
}

static inline void established_socket_set_read_partial(struct established_socket * this, bool read_partial)
{
	assert(this != NULL);
	this->flags.read_partial = read_partial;
}

bool established_socket_ssl_enable(struct established_socket *, SSL_CTX *ctx);

//

struct frame * get_read_frame_simple(struct established_socket * this);

#endif // __LIBSCCMN_SOCK_EST_H__
