#include "all.h"

///

static bool listen_on_accept(struct ft_listener * listening_socket, int fd, const struct sockaddr * client_addr, socklen_t client_addr_len, bool (*connection_init)(struct connection * , struct ft_listener * listening_socket, int fd, const struct sockaddr * peer_addr, socklen_t peer_addr_len))
{
	bool ok;

	struct ft_list_node * new_node = ft_list_node_new(sizeof(struct connection));
	if (new_node == NULL) return false;

	struct connection * connection = (struct connection *)&new_node->data;

	ok = connection_init(connection, listening_socket, fd, client_addr, client_addr_len);
	if (!ok)
	{
		ft_list_node_del(new_node);
		return false;
	}

	ft_list_add(&app.connections, new_node);

	return true;
}

///

static bool on_accept_http_cb(struct ft_listener * listening_socket, int fd, const struct sockaddr * client_addr, socklen_t client_addr_len)
{
	return listen_on_accept(listening_socket, fd, client_addr, client_addr_len, connection_init_http);
}

static struct ft_listener_delegate listener_http_delegate =
{
	.accept = on_accept_http_cb,
};

///

static bool on_accept_https_cb(struct ft_listener * listening_socket, int fd, const struct sockaddr * client_addr, socklen_t client_addr_len)
{
	return listen_on_accept(listening_socket, fd, client_addr, client_addr_len, connection_init_https);
}

static struct ft_listener_delegate listener_https_delegate =
{
	.accept = on_accept_https_cb,
};


///

static void on_exit_cb(struct ft_exit * exit, struct ft_context * context, enum ft_exit_phase phase)
{
	struct listen * this = exit->data;
	assert(this != NULL);

	ft_listener_list_cntl(&this->listeners, FT_LISTENER_STOP);
}

///

bool listen_init(struct listen * this, struct ft_context * context)
{
	assert(this != NULL);

	// Prepare listening socket(s)
	ft_listener_list_init(&this->listeners);

	// Install termination handler
	ft_exit_init(&this->exit, context, on_exit_cb);
	this->exit.data = this;

	return true;
}

void listen_fini(struct listen * this)
{
	ft_list_fini(&this->listeners);
}

///

bool listen_extend_http(struct listen * this, struct ft_context * context, const char * value)
{
	int rc;

	assert(this != NULL);
	assert(context != NULL);

	rc = ft_listener_list_extend_auto(&this->listeners, &listener_http_delegate, context, SOCK_STREAM, value);
	return (rc >= 0);
}

bool listen_extend_https(struct listen * this, struct ft_context * context, const char * value)
{
	int rc;

	assert(this != NULL);
	assert(context != NULL);

	rc = ft_listener_list_extend_auto(&this->listeners, &listener_https_delegate, context, SOCK_STREAM, value);
	return (rc >= 0);
}


bool listen_start(struct listen * this)
{
	assert(this != NULL);

	// Start listening
	return ft_listener_list_cntl(&this->listeners, FT_LISTENER_START);
}

bool listen_stop(struct listen * this)
{
	assert(this != NULL);

	// Start listening
	return ft_listener_list_cntl(&this->listeners, FT_LISTENER_STOP);
}

