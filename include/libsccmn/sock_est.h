#ifndef __LIBSCCMN_SOCK_EST_H__
#define __LIBSCCMN_SOCK_EST_H__

struct established_socket;

struct established_socket_cb
{
	size_t (*read_advise)(struct established_socket *, struct frame * frame);

	// True as a return value means, that the frame has been handed over to upstream protocol
	bool (*read)(struct established_socket *, struct frame * frame);

	void (*state_changed)(struct established_socket *);
};

enum established_socket_state
{
	established_socket_state_INIT = '?',
	established_socket_state_ESTABLISHED = 'E',
	established_socket_state_SHUTDOWN = 'C',
	established_socket_state_CLOSED = 'c',
};

struct established_socket
{
	struct context * context;

	// Input
	struct ev_io read_watcher;
	struct frame * read_frame;

	// Output
	bool writeable;
	struct ev_io write_watcher;
	struct frame * write_frames; // Queue of write frames 
	struct frame ** write_frame_last;

	// Common fields
	enum established_socket_state state;

	int ai_family;
	int ai_socktype;
	int ai_protocol;

	struct sockaddr_storage ai_addr;
	socklen_t ai_addrlen;

	struct established_socket_cb * cbs;

	ev_tstamp establish_at;
	ev_tstamp shutdown_at;

	// Statistics
	struct
	{
		unsigned int read_events;
		unsigned int write_events;
		unsigned long read_bytes;
		unsigned long write_bytes;
	} stats;

	// Custom data field
	void * data;
};

bool established_socket_init_accept(struct established_socket *, struct established_socket_cb * cbs, struct listening_socket * listening_socket, int fd, const struct sockaddr * peer_addr, socklen_t peer_addr_len);
void established_socket_fini(struct established_socket *);

bool established_socket_read_start(struct established_socket *);
bool established_socket_read_stop(struct established_socket *);

bool established_socket_write_start(struct established_socket *);
bool established_socket_write_stop(struct established_socket *);

bool established_socket_write(struct established_socket *, struct frame * frame);

bool established_socket_shutdown(struct established_socket *);


///

size_t established_socket_read_advise_max_frame(struct established_socket *, struct frame * frame);

#endif // __LIBSCCMN_SOCK_EST_H__
