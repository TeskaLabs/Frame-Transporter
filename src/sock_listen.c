#include "all.h"

static void listening_socket_on_io(struct ev_loop *loop, struct ev_io *watcher, int revents);

///

bool listening_socket_init(struct listening_socket * this, struct addrinfo * rp, listening_socket_cb cb, int backlog)
{
	int rc;
	int fd = -1;

	this->listening = false;
	this->data = NULL;
	this->cb = cb;
	this->backlog = backlog;

	this->ai_family = rp->ai_family;
	this->ai_socktype = rp->ai_socktype;
	this->ai_protocol = rp->ai_protocol;
	memcpy(&this->ai_addr, &rp->ai_addr, rp->ai_addrlen);
	this->ai_addrlen = rp->ai_addrlen;

	char hoststr[NI_MAXHOST];
	char portstr[NI_MAXSERV];
	rc = getnameinfo(rp->ai_addr, sizeof(struct sockaddr), hoststr, sizeof(hoststr), portstr, sizeof(portstr), NI_NUMERICHOST | NI_NUMERICSERV);
	if (rc != 0)
	{
		if (rc == EAI_FAMILY)
		{
			// This is not a failure
			return false;
		}

		L_WARN("Problem occured when resolving listen socket: %s", gai_strerror(rc));
		strcpy(hoststr, "?");
		strcpy(portstr, "?");
	}

	// Create socket
	fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
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
	rc = bind(fd, rp->ai_addr, rp->ai_addrlen);
	if (rc != 0)
	{
		L_ERROR_ERRNO(errno, "bind to %s %s", hoststr, portstr);
		goto error_exit;
	}

	ev_io_init(&this->watcher, listening_socket_on_io, fd, EV_READ);
	this->watcher.data = this;

	L_DEBUG("Listening on %s %s", hoststr, portstr);
	return true;

error_exit:
	if (fd >= 0) close(fd);
	return false;
}


void listening_socket_close(struct listening_socket * this)
{
	int rc;

	if (this->watcher.fd >= 0)
	{
		ev_io_stop(libsccmn_config.ev_loop, &this->watcher);
		rc = close(this->watcher.fd);
		if (rc != 0) L_ERROR_ERRNO(errno, "close()");
		this->watcher.fd = -1;
	}
}


bool listening_socket_start(struct ev_loop * loop, struct listening_socket * this)
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

	ev_io_start(loop, &this->watcher);
	return true;
}


bool listening_socket_stop(struct ev_loop * loop, struct listening_socket * this)
{
	if (this->watcher.fd < 0)
	{
		L_WARN("Listening on socket that is not open!");
		return false;
	}

	ev_io_stop(loop, &this->watcher);
	return true;
}


static void listening_socket_on_io(struct ev_loop * loop, struct ev_io *watcher, int revents)
{
	struct listening_socket * this = watcher->data;

	if (revents & EV_ERROR)
	{
		L_ERROR("Listen socket (accept) got invalid event");
		return;
	}

	if (revents & EV_READ)
	{
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

		if (this->cb == NULL)
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

		this->cb(loop, this, client_socket, (const struct sockaddr *)&client_addr, client_len);
	}

}

///

int listening_socket_chain_extend(struct listening_socket_chain ** chain, struct addrinfo * addrinfo, listening_socket_cb cb, int backlog)
{
	int count = 0;
	struct listening_socket_chain ** p = chain;
	
	// Iterate to the end of the chain
	while (*p != NULL)
	{
		p = &((*p)->next);
	}

	for (struct addrinfo * rp = addrinfo; rp != NULL; rp = rp->ai_next)
	{
		struct listening_socket_chain * chitem = malloc(sizeof(struct listening_socket_chain));
		bool ok = listening_socket_init(&chitem->listening_socket, rp, cb, backlog);
		if (!ok)
		{
			free(chitem);
			continue;
		}

		*p = chitem;
		chitem->next = NULL;
		p = &chitem->next;
		count += 1;
	}

	return count;
}


void listening_socket_chain_start(struct ev_loop * loop, struct listening_socket_chain * chain)
{
	for (struct listening_socket_chain * i = chain; i != NULL; i = i->next)
		listening_socket_start(loop, &i->listening_socket);
}


void listening_socket_chain_stop(struct ev_loop * loop, struct listening_socket_chain * chain)
{
	for (struct listening_socket_chain * i = chain; i != NULL; i = i->next)
		listening_socket_stop(loop, &i->listening_socket);
}


void listening_socket_chain_set_data(struct listening_socket_chain * chain, void * data)
{
	for (struct listening_socket_chain * i = chain; i != NULL; i = i->next)
		i->listening_socket.data = data;
}


void listening_socket_chain_del(struct listening_socket_chain * chain)
{
	while (chain != NULL)
	{
		struct listening_socket_chain * i = chain; 
		chain = i->next;
		listening_socket_close(&i->listening_socket);
		free(i);
	}
}

