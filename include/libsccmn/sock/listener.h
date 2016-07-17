#ifndef FT_SOCK_LISTENER_H_
#define FT_SOCK_LISTENER_H_

struct listening_socket;

struct ft_listener_delegate
{
	bool (* accept)(struct listening_socket *, int fd, const struct sockaddr * client_addr, socklen_t client_addr_len);
};


struct listening_socket
{
	struct ft_listener_delegate * delegate;
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

	struct
	{
		unsigned int accept_events;
	} stats;

	// Custom data field
	void * data;
};

bool listening_socket_init(struct listening_socket * , struct ft_listener_delegate * delegate, struct context * context, struct addrinfo * rp);
void listening_socket_fini(struct listening_socket *);

///

enum ft_listener_cntl_codes
{
	FT_LISTENER_START = 0x0001,
	FT_LISTENER_STOP = 0x0002,
};


static inline bool ft_listener_cntl(struct listening_socket * this, const int control_code)
{
	assert(this != NULL);

	bool _ft_listener_cntl_start(struct listening_socket *);
	bool _ft_listener_cntl_stop(struct listening_socket *);

	bool ok = true;
	if ((control_code & FT_LISTENER_START) != 0) ok &= _ft_listener_cntl_start(this);
	if ((control_code & FT_LISTENER_STOP) != 0) ok &= _ft_listener_cntl_stop(this);
	return ok;
}

///

bool ft_listener_list_init(struct ft_list *);

int ft_listener_list_extend(struct ft_list *, struct ft_listener_delegate * delegate, struct context * context, int ai_family, int ai_socktype, const char * host, const char * port);
int ft_listener_list_extend_by_addrinfo(struct ft_list *, struct ft_listener_delegate * delegate, struct context * context, struct addrinfo * rp_list);

bool ft_listener_list_cntl(struct ft_list *, const int control_code);

#endif // FT_SOCK_LISTENER_H_
