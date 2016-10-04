#ifndef FT_SOCK_SOCKET_H_
#define FT_SOCK_SOCKET_H_

struct ft_socket
{
	struct ft_context * context;

	const char * socket_class;

	int ai_family;
	int ai_socktype;
	int ai_protocol;

	struct sockaddr_storage addr;
	socklen_t addrlen;

	// Custom data fields
	void * protocol;
	void * data;
};


static inline bool ft_socket_init_(
	struct ft_socket * this, const char * socket_class, struct ft_context * context,
	int domain, int type, int protocol,
	const struct sockaddr * addr, socklen_t addrlen
)
{
	assert(this != NULL);
	assert(context != NULL);


	this->context = context;
	this->socket_class = socket_class;

	this->ai_family = domain;
	this->ai_socktype = type;
	this->ai_protocol = protocol;

	if (addr != NULL)
	{
		memcpy(&this->addr, addr, addrlen);
		this->addrlen = addrlen;
	}
	else
	{
		assert(addrlen == 0);
		memset(&this->addr, 0, sizeof(this->addr));
	}

	this->protocol = NULL;
	this->data = NULL;

	return true;
}

#endif //FT_SOCK_SOCKET_H_
