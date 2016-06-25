#include "all.h"

struct context context;
struct exiting_watcher watcher;

///

bool on_read(struct established_socket * established_sock, struct frame * frame)
{
//	frame_print(frame);	
	frame_flip(frame);
	bool ok = established_socket_write(established_sock, frame);
	if (!ok)
	{
		L_ERROR("Cannot write a frame!");
		frame_pool_return(frame);
	}

	return true;
}

struct established_socket_cb sock_est_sock_cb = 
{
	.get_read_frame = get_read_frame_simple,
	.read = on_read,
	.state_changed = NULL,
};

///

struct listening_socket_chain * lsocks = NULL;
struct established_socket established_sock; //TODO: Replace by some kind of list

static bool on_accept_cb(struct listening_socket * listening_socket, int fd, const struct sockaddr * client_addr, socklen_t client_addr_len)
{
	bool ok;
	ok = established_socket_init_accept(&established_sock, &sock_est_sock_cb, listening_socket, fd, client_addr, client_addr_len);
	if (!ok) return false;

	established_sock.read_opportunistic = true;

	established_socket_read_start(&established_sock);

	return true;
}

struct listening_socket_cb sock_listen_sock_cb =
{
	.accept = on_accept_cb,
};


///

static void on_exiting_cb(struct exiting_watcher * watcher, struct context * context)
{
	listening_socket_chain_stop(lsocks);
}

int main(int argc, char const *argv[])
{
	bool ok;
	int rc;
	
	logging_set_verbose(true);
	libsccmn_init();
	
	ok = context_init(&context);
	if (!ok) return EXIT_FAILURE;

	// Start listening
	rc = listening_socket_chain_extend_getaddrinfo(&lsocks, &sock_listen_sock_cb, &context, AF_INET, SOCK_STREAM, "127.0.0.1", "12345");
	if (rc < 0) return EXIT_FAILURE;

	rc = listening_socket_chain_extend_getaddrinfo(&lsocks, &sock_listen_sock_cb, &context, AF_UNIX, SOCK_STREAM, "/tmp/sctext.tmp", NULL);
	if (rc < 0) return EXIT_FAILURE;

	listening_socket_chain_start(lsocks);

	context_exiting_watcher_add(&context, &watcher, on_exiting_cb);

	context_evloop_run(&context);

	listening_socket_chain_del(lsocks);

	context_fini(&context);

	return EXIT_SUCCESS;
}
