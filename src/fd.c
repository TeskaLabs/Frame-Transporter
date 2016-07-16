#include "all.h"

bool set_socket_nonblocking(int fd)
{
	int flags = fcntl(fd, F_GETFL, 0);
  	if (flags < 0) return false;
	flags |= O_NONBLOCK;
	return (fcntl(fd, F_SETFL, flags) == 0) ? true : false;
}


bool set_tcp_keepalive(int fd)
{
	int optval = 1;
	socklen_t optlen = sizeof(optval);

	if(setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &optval, optlen) < 0)
	{
	    L_WARN_ERRNO(errno, "Error activating SO_KEEPALIVE on a socket (fd: %d)", fd);
	    return false;
	}

//TODO: This ...
#ifdef TCP_KEEPIDLE
	optval = CONFIG->TCP_KEEPALIVE_TIME;
	optlen = sizeof(optval);
	if(setsockopt(fd, SOL_TCP, TCP_KEEPIDLE, &optval, optlen) < 0)
	{
		L_WARN_ERRNO(errno, "Error setting TCP_KEEPIDLE on a socket (fd: %d)", fd);
	}
#endif

	return true;
}


bool set_close_on_exec(int fd)
{
	// Fetch flags
	int flags = fcntl(fd, F_GETFD);
	if (flags == -1)
	{
		//TODO: L_ERROR_ERRNO(errno, "close-on-exec/fcntl(F_GETFD)");
		return false;
	}

	// Turn on FD_CLOEXEC
	flags |= FD_CLOEXEC;

	// Update flags
	int rc = fcntl(fd, F_SETFD, flags);
	if (rc == -1)   
	{
		//TODO: L_ERROR_ERRNO(errno, "close-on-exec/fcntl(F_SETFD)");
		return false;
	}

	return true;
}
