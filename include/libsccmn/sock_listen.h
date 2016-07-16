#ifndef __LIBSCCMN_SOCK_LISTEN_H__
#define __LIBSCCMN_SOCK_LISTEN_H__

struct listening_socket;

struct listening_socket_cb
{
	bool (* accept)(struct listening_socket *, int fd, const struct sockaddr * client_addr, socklen_t client_addr_len);
};


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

	struct listening_socket_cb * cbs;

	struct
	{
		unsigned int accept_events;
	} stats;

	// Custom data field
	void * data;
};

bool listening_socket_init(struct listening_socket * , struct listening_socket_cb * cbs, struct context * context, struct addrinfo * rp);
void listening_socket_fini(struct listening_socket *);

bool listening_socket_start(struct listening_socket *);
bool listening_socket_stop(struct listening_socket *);

///

typedef bool (*listening_socket_getaddrinfo_cb)(void * data, struct context * context, struct addrinfo * addrinfo);

int listening_socket_create_getaddrinfo(listening_socket_getaddrinfo_cb, void * data, struct context * context, int ai_family, int ai_socktype, const char * host, const char * port);

#endif //__LIBSCCMN_SOCK_LISTEN_H__
