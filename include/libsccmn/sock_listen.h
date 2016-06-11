#ifndef __LIBSCCMN_SOCK_LISTEN_H__
#define __LIBSCCMN_SOCK_LISTEN_H__

struct listening_socket;

typedef void (* listening_socket_cb)(struct listening_socket *, int fd, const struct sockaddr * client_addr, socklen_t client_addr_len);

struct listening_socket
{
	struct context * context;
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

bool listening_socket_init(struct listening_socket * , struct context * context, struct addrinfo * rp, listening_socket_cb cb);

bool listening_socket_start(struct listening_socket *);
bool listening_socket_stop(struct listening_socket *);

void listening_socket_close(struct listening_socket *);

///

struct listening_socket_chain
{
	struct listening_socket_chain * next;
	struct listening_socket listening_socket;
};

int listening_socket_chain_extend(struct listening_socket_chain ** chain, struct context * context, struct addrinfo * addrinfo, listening_socket_cb cb);
void listening_socket_chain_del(struct listening_socket_chain *);

void listening_socket_chain_start(struct listening_socket_chain *);
void listening_socket_chain_stop(struct listening_socket_chain *);

void listening_socket_chain_set_data(struct listening_socket_chain *, void * data);

#endif //__LIBSCCMN_SOCK_LISTEN_H__
