#include "all.h"
#include <sys/un.h>

static void listening_socket_on_io(struct ev_loop *loop, struct ev_io *watcher, int revents);

///

bool listening_socket_init(struct listening_socket * this, struct listening_socket_cb * cbs, struct context * context, struct addrinfo * ai)
{
	int rc;
	int fd = -1;

	assert(cbs != NULL);
	assert(context != NULL);
	this->context = context;

	this->listening = false;
	this->data = NULL;
	this->cbs = cbs;
	this->backlog = libsccmn_config.sock_listen_backlog;

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
				L_WARN("Unsupported family: %d", this->ai_family);
				// This is not a failure
				return false;
			}

			L_WARN("Problem occured when resolving listen socket: %s", gai_strerror(rc));
			snprintf(addrstr, sizeof(addrstr)-1, "? ?");
		}

		else
			snprintf(addrstr, sizeof(addrstr)-1, "%s %s", hoststr, portstr);
	}

	// Create socket
	fd = socket(this->ai_family, this->ai_socktype, this->ai_protocol);
	if (fd < 0)
	{
		L_ERROR_ERRNO(errno, "Failed creating listen socket");
		goto error_exit;
	}

	// Set reuse address option
	int on = 1;
	rc = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const char *) &on, sizeof(on));
	if (rc == -1)
	{
		L_ERROR_ERRNO(errno, "Failed when setting option SO_REUSEADDR to listen socket");
		goto error_exit;
	}

	bool res = set_socket_nonblocking(fd);
	if (!res) L_WARN_ERRNO(errno, "Failed when setting listen socket to non-blocking mode");

#if TCP_DEFER_ACCEPT
	// See http://www.techrepublic.com/article/take-advantage-of-tcp-ip-options-to-optimize-data-transmission/
	int timeout = 1;
	rc = setsockopt(fd, IPPROTO_TCP, TCP_DEFER_ACCEPT, &timeout, sizeof(int));
	if (rc == -1) L_WARN_ERRNO(errno, "Failed when setting option TCP_DEFER_ACCEPT to listen socket");
#endif // TCP_DEFER_ACCEPT

	// Bind socket
	rc = bind(fd, (const struct sockaddr *)&this->ai_addr, this->ai_addrlen);
	if (rc != 0)
	{
		L_ERROR_ERRNO(errno, "bind to %s ", addrstr);
		goto error_exit;
	}

	ev_io_init(&this->watcher, listening_socket_on_io, fd, EV_READ);
	this->watcher.data = this;

	L_DEBUG("Listening on %s", addrstr);
	return true;

error_exit:
	if (fd >= 0) close(fd);
	return false;
}


void listening_socket_fini(struct listening_socket * this)
{
	int rc;

	if (this->watcher.fd >= 0)
	{
		ev_io_stop(this->context->ev_loop, &this->watcher);
		rc = close(this->watcher.fd);
		if (rc != 0) L_ERROR_ERRNO(errno, "close()");
		this->watcher.fd = -1;
	}

	if (this->ai_family == AF_UNIX)
	{
		struct sockaddr_un * un = (struct sockaddr_un *)&this->ai_addr;
		
		rc = unlink(un->sun_path);
		if (rc != 0) L_WARN_ERRNO(errno, "Unlinking unix socket '%s'", un->sun_path);
	}
}


bool listening_socket_start(struct listening_socket * this)
{
	int rc;

	if (this->watcher.fd < 0)
	{
		L_WARN("Listening on socket that is not open!");
		return false;
	}

	if (!this->listening)
	{
		rc = listen(this->watcher.fd, this->backlog);
		if (rc != 0)
		{
			L_ERROR_ERRNO_P(errno, "listen(%d, %d)", this->watcher.fd, this->backlog);
			return false;
		}
		this->listening = true;
	}

	ev_io_start(this->context->ev_loop, &this->watcher);
	return true;
}


bool listening_socket_stop(struct listening_socket * this)
{
	if (this->watcher.fd < 0)
	{
		L_WARN("Listening (stop) on socket that is not open!");
		return false;
	}

	ev_io_stop(this->context->ev_loop, &this->watcher);
	return true;
}


static void listening_socket_on_io(struct ev_loop * loop, struct ev_io *watcher, int revents)
{
	struct listening_socket * this = watcher->data;
	assert(this != NULL);

	if (revents & EV_ERROR)
	{
		L_ERROR("Listen socket (accept) got invalid event");
		return;
	}

	if ((revents & EV_READ) == 0) return;

	this->stats.accept_events += 1;

	struct sockaddr_storage client_addr;
	socklen_t client_len = sizeof(struct sockaddr_storage);

	// Accept client request
	int client_socket = accept(watcher->fd, (struct sockaddr *)&client_addr, &client_len);
	if (client_socket < 0)
	{
		if ((errno == EAGAIN) || (errno==EWOULDBLOCK)) return;
		L_ERROR_ERRNO(errno, "Accept on listening socket");
		return;
	}

	if (this->cbs->accept == NULL)
	{
		close(client_socket);
		return;
	}

/*
	char host[NI_MAXHOST];
	char port[NI_MAXSERV];

	rc = getnameinfo((struct sockaddr *)&client_addr, client_len, host, sizeof(host), port, sizeof(port), NI_NUMERICHOST | NI_NUMERICSERV);
	if (rc != 0)
	{
		L_WARN("Failed to resolve address of accepted connection: %s", gai_strerror(rc));
		strcpy(host, "?");
		strcpy(port, "?");
	}
*/

	bool ok = this->cbs->accept(this, client_socket, (const struct sockaddr *)&client_addr, client_len);
	if (!ok) close(client_socket);

}

///

int listening_socket_create_getaddrinfo(listening_socket_getaddrinfo_cb cb, void * data, struct context * context, int ai_family, int ai_socktype, const char * host, const char * port)
{
	assert(cb != NULL);

	bool ok;
	int rc;
	struct addrinfo hints;

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

		ok = cb(data, context, &hints);
		rc = ok ? 1 : 0;
	}

	else
	{
		struct addrinfo * res;

		rc = getaddrinfo(host, port, &hints, &res);
		if (rc != 0)
		{
			L_ERROR("getaddrinfo failed: %s", gai_strerror(rc));
			return -1;
		}

		int rc = 0;
		for (struct addrinfo * rp = res; rp != NULL; rp = rp->ai_next)
		{
			ok = cb(data, context, rp);
			rc += ok ? 1 : 0;
		}

		freeaddrinfo(res);
	}

	return rc;
}
