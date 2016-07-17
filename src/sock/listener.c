#include "../_ft_internal.h"
#include <sys/un.h>

static void _ft_listener_on_io(struct ev_loop *loop, struct ev_io *watcher, int revents);

///

bool ft_listener_init(struct ft_listener * this, struct ft_listener_delegate * delegate, struct ft_context * context, struct addrinfo * ai)
{
	int rc;
	int fd = -1;

	assert(this != NULL);
	assert(delegate != NULL);
	assert(context != NULL);
	
	FT_TRACE(FT_TRACE_ID_LISTENER, "BEGIN");

	this->delegate = delegate;
	this->context = context;
	this->listening = false;
	this->data = NULL;
	this->backlog = ft_config.sock_listen_backlog;

	this->ai_family = ai->ai_family;
	this->ai_socktype = ai->ai_socktype;
	this->ai_protocol = ai->ai_protocol;
	memcpy(&this->ai_addr, ai->ai_addr, ai->ai_addrlen);
	this->ai_addrlen = ai->ai_addrlen;

	this->stats.accept_events = 0;

	char addrstr[NI_MAXHOST+NI_MAXSERV];

	if (this->ai_family == AF_UNIX)
	{
		struct sockaddr_un * un = (struct sockaddr_un *)&this->ai_addr;
		strcpy(addrstr, un->sun_path);
	}

	else
	{
		char hoststr[NI_MAXHOST];
		char portstr[NI_MAXHOST];

		rc = getnameinfo((const struct sockaddr *)&this->ai_addr, this->ai_addrlen, hoststr, sizeof(hoststr), portstr, sizeof(portstr), NI_NUMERICHOST | NI_NUMERICSERV);
		if (rc != 0)
		{
			if (rc == EAI_FAMILY)
			{
				FT_WARN("Unsupported family: %d", this->ai_family);
				// This is not a failure
				FT_TRACE(FT_TRACE_ID_LISTENER, "END EAI_FAMILY");
				return false;
			}

			FT_WARN("Problem occured when resolving listen socket: %s", gai_strerror(rc));
			snprintf(addrstr, sizeof(addrstr)-1, "? ?");
		}

		else
			snprintf(addrstr, sizeof(addrstr)-1, "%s %s", hoststr, portstr);
	}

	// Create socket
	fd = socket(this->ai_family, this->ai_socktype, this->ai_protocol);
	if (fd < 0)
	{
		FT_ERROR_ERRNO(errno, "Failed creating listen socket");
		goto error_exit;
	}

	// Set reuse address option
	int on = 1;
	rc = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const char *) &on, sizeof(on));
	if (rc == -1)
	{
		FT_ERROR_ERRNO(errno, "Failed when setting option SO_REUSEADDR to listen socket");
		goto error_exit;
	}

	bool res = ft_fd_nonblock(fd);
	if (!res) FT_WARN_ERRNO(errno, "Failed when setting listen socket to non-blocking mode");

#if TCP_DEFER_ACCEPT
	// See http://www.techrepublic.com/article/take-advantage-of-tcp-ip-options-to-optimize-data-transmission/
	int timeout = 1;
	rc = setsockopt(fd, IPPROTO_TCP, TCP_DEFER_ACCEPT, &timeout, sizeof(int));
	if (rc == -1) FT_WARN_ERRNO(errno, "Failed when setting option TCP_DEFER_ACCEPT to listen socket");
#endif // TCP_DEFER_ACCEPT

	// Bind socket
	rc = bind(fd, (const struct sockaddr *)&this->ai_addr, this->ai_addrlen);
	if (rc != 0)
	{
		FT_ERROR_ERRNO(errno, "bind to %s ", addrstr);
		goto error_exit;
	}

	ev_io_init(&this->watcher, _ft_listener_on_io, fd, EV_READ);
	this->watcher.data = this;

	FT_DEBUG("Listening on %s", addrstr);
	FT_TRACE(FT_TRACE_ID_LISTENER, "END fd:%d", fd);
	return true;

error_exit:
	if (fd >= 0) close(fd);

	FT_TRACE(FT_TRACE_ID_LISTENER, "END error");
	return false;
}


void ft_listener_fini(struct ft_listener * this)
{
	int rc;

	FT_TRACE(FT_TRACE_ID_LISTENER, "BEGIN fd:%d", this->watcher.fd);

	if (this->watcher.fd >= 0)
	{
		ev_io_stop(this->context->ev_loop, &this->watcher);
		rc = close(this->watcher.fd);
		if (rc != 0) FT_ERROR_ERRNO(errno, "close()");
		this->watcher.fd = -1;
	}

	if (this->ai_family == AF_UNIX)
	{
		struct sockaddr_un * un = (struct sockaddr_un *)&this->ai_addr;
		
		rc = unlink(un->sun_path);
		if (rc != 0) FT_WARN_ERRNO(errno, "Unlinking unix socket '%s'", un->sun_path);
	}

	FT_TRACE(FT_TRACE_ID_LISTENER, "END");
}


bool _ft_listener_cntl_start(struct ft_listener * this)
{
	int rc;

	FT_TRACE(FT_TRACE_ID_LISTENER, "BEGIN fd:%d", this->watcher.fd);

	if (this->watcher.fd < 0)
	{
		FT_WARN("Listening on socket that is not open!");
		FT_TRACE(FT_TRACE_ID_LISTENER, "END not open fd:%d", this->watcher.fd);
		return false;
	}

	if (!this->listening)
	{
		rc = listen(this->watcher.fd, this->backlog);
		if (rc != 0)
		{
			FT_ERROR_ERRNO_P(errno, "listen(%d, %d)", this->watcher.fd, this->backlog);
			FT_TRACE(FT_TRACE_ID_LISTENER, "END listen error fd:%d", this->watcher.fd);
			return false;
		}
		this->listening = true;
	}

	ev_io_start(this->context->ev_loop, &this->watcher);
	FT_TRACE(FT_TRACE_ID_LISTENER, "END fd:%d", this->watcher.fd);
	return true;
}


bool _ft_listener_cntl_stop(struct ft_listener * this)
{
	FT_TRACE(FT_TRACE_ID_LISTENER, "BEGIN fd:%d", this->watcher.fd);

	if (this->watcher.fd < 0)
	{
		FT_WARN("Listening (stop) on socket that is not open!");
		FT_TRACE(FT_TRACE_ID_LISTENER, "END not open fd:%d", this->watcher.fd);
		return false;
	}

	ev_io_stop(this->context->ev_loop, &this->watcher);
	FT_TRACE(FT_TRACE_ID_LISTENER, "END fd:%d", this->watcher.fd);
	return true;
}


static void _ft_listener_on_io(struct ev_loop * loop, struct ev_io *watcher, int revents)
{
	struct ft_listener * this = watcher->data;
	assert(this != NULL);

	FT_TRACE(FT_TRACE_ID_LISTENER, "BEGIN fd:%d", this->watcher.fd);

	if (revents & EV_ERROR)
	{
		FT_ERROR("Listen socket (accept) got invalid event");
		FT_TRACE(FT_TRACE_ID_LISTENER, "END ev error fd:%d", this->watcher.fd);
		return;
	}

	if ((revents & EV_READ) == 0)
	{
		FT_TRACE(FT_TRACE_ID_LISTENER, "END noop fd:%d", this->watcher.fd);
		return;
	}

	this->stats.accept_events += 1;

	struct sockaddr_storage client_addr;
	socklen_t client_len = sizeof(struct sockaddr_storage);

	// Accept client request
	int client_socket = accept(watcher->fd, (struct sockaddr *)&client_addr, &client_len);
	if (client_socket < 0)
	{
		if ((errno == EAGAIN) || (errno==EWOULDBLOCK))
		{
			FT_TRACE(FT_TRACE_ID_LISTENER, "END fd:%d", this->watcher.fd);
			return;
		}
		int errnum = errno;
		FT_ERROR_ERRNO(errnum, "Accept on listening socket");
		FT_TRACE(FT_TRACE_ID_LISTENER, "END fd:%d errno:%d", this->watcher.fd, errnum);
		return;
	}

	if (this->delegate->accept == NULL)
	{
		close(client_socket);
		FT_TRACE(FT_TRACE_ID_LISTENER, "END fd:%d ", this->watcher.fd);
		return;
	}

/*
	char host[NI_MAXHOST];
	char port[NI_MAXSERV];

	rc = getnameinfo((struct sockaddr *)&client_addr, client_len, host, sizeof(host), port, sizeof(port), NI_NUMERICHOST | NI_NUMERICSERV);
	if (rc != 0)
	{
		FT_WARN("Failed to resolve address of accepted connection: %s", gai_strerror(rc));
		strcpy(host, "?");
		strcpy(port, "?");
	}
*/

	bool ok = this->delegate->accept(this, client_socket, (const struct sockaddr *)&client_addr, client_len);
	if (!ok) close(client_socket);

	FT_TRACE(FT_TRACE_ID_LISTENER, "END fd:%d cfd:%d ok:%c", this->watcher.fd, client_socket, ok ? 'Y' : 'N');
}

///

static void ft_listener_list_on_remove(struct ft_list * list, struct ft_list_node * node)
{
	assert(list != NULL);

	ft_listener_fini((struct ft_listener *)node->data);
}

bool ft_listener_list_init(struct ft_list * list)
{
	assert(list != NULL);

	return ft_list_init(list, ft_listener_list_on_remove);
}


bool ft_listener_list_cntl(struct ft_list * list, const int control_code)
{
	assert(list != NULL);

	bool ok = true;
	FT_LIST_FOR(list, node)
	{
		ok &= ft_listener_cntl((struct ft_listener *)node->data, control_code);
	}
	return ok;
}


int ft_listener_list_extend(struct ft_list * list, struct ft_listener_delegate * delegate, struct ft_context * context, int ai_family, int ai_socktype, const char * host, const char * port)
{
	assert(list != NULL);
	assert(delegate != NULL);
	assert(context != NULL);

	int rc;
	struct addrinfo hints;

	FT_TRACE(FT_TRACE_ID_LISTENER, "BEGIN");

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = ai_family;
	hints.ai_socktype = ai_socktype;
	hints.ai_flags = AI_PASSIVE;
	hints.ai_protocol = 0;
	hints.ai_canonname = NULL;
	hints.ai_addr = NULL;
	hints.ai_next = NULL;

	if (ai_family == AF_UNIX)
	{
		struct sockaddr_un un;

		un.sun_family = AF_UNIX;
		snprintf(un.sun_path, sizeof(un.sun_path)-1, "%s", host);
		hints.ai_addr = (struct sockaddr *)&un;
		hints.ai_addrlen = sizeof(un);

		rc = ft_listener_list_extend_by_addrinfo(list, delegate, context, &hints);
	}

	else
	{
		struct addrinfo * res;

		rc = getaddrinfo(host, port, &hints, &res);
		if (rc != 0)
		{
			FT_ERROR("getaddrinfo failed: %s", gai_strerror(rc));
			return -1;
		}

		rc = ft_listener_list_extend_by_addrinfo(list, delegate, context, res);

		freeaddrinfo(res);
	}

	FT_TRACE(FT_TRACE_ID_LISTENER, "END rc:%d", rc);

	return rc;
}

int ft_listener_list_extend_by_addrinfo(struct ft_list * list, struct ft_listener_delegate * delegate, struct ft_context * context, struct addrinfo * rp_list)
{
	assert(list != NULL);
	assert(delegate != NULL);
	assert(context != NULL);

	FT_TRACE(FT_TRACE_ID_LISTENER, "BEGIN");

	int rc = 0;
	for (struct addrinfo * rp = rp_list; rp != NULL; rp = rp->ai_next)
	{

		struct ft_list_node * new_node = ft_list_node_new(sizeof(struct ft_listener));
		if (new_node == NULL)
		{
			FT_WARN("Failed to allocate memory for a new listener");
			continue;
		}

		bool ok = ft_listener_init((struct ft_listener *)new_node->data, delegate, context, rp);
		if (!ok) 
		{
			ft_list_node_del(new_node);
			FT_WARN("Failed to initialize a new listener");
			continue;
		}

		ft_list_add(list, new_node);

		rc += 1;
	}

	FT_TRACE(FT_TRACE_ID_LISTENER, "END rc:%d", rc);

	return rc;
}
