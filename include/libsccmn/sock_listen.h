#ifndef __LIBSCCMN_LISTENING_H__
#define __LIBSCCMN_LISTENING_H__

struct listening_socket;

typedef void (* listening_socket_cb)(struct ev_loop * loop, struct listening_socket *, int fd, struct sockaddr_storage * client_addr, socklen_t client_addr_len);

struct listening_socket
{
	struct ev_io watcher;

	// Following members are from struct addrinfo
	int ai_family;
	int ai_socktype;
	int ai_protocol;

	bool listening;
	int backlog;

	struct sockaddr_storage ai_addr;
	socklen_t ai_addrlen;

	// Callback
	listening_socket_cb cb;

	// Custom data field
	void * data;
};

bool listening_socket_init(struct listening_socket *, struct addrinfo * rp, listening_socket_cb cb, int backlog);

bool listening_socket_start(struct ev_loop * loop, struct listening_socket *);
bool listening_socket_stop(struct ev_loop * loop, struct listening_socket *);

void listening_socket_close(struct listening_socket *);

///

struct listening_socket_chain
{
	struct listening_socket_chain * next;
	struct listening_socket listening_socket;
};

int listening_socket_chain_extend(struct listening_socket_chain ** chain, struct addrinfo * addrinfo, listening_socket_cb cb, int backlog);
void listening_socket_chain_del(struct listening_socket_chain *);

void listening_socket_chain_start(struct ev_loop * loop, struct listening_socket_chain *);
void listening_socket_chain_stop(struct ev_loop * loop, struct listening_socket_chain *);

#endif //__LIBSCCMN_LISTENING_H__
