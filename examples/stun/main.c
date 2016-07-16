#include <stdlib.h>
#include <stdbool.h>

#include <libsccmn.h>

///

struct exiting_watcher watcher;
struct addrinfo * target_addr;
SSL_CTX * ssl_ctx;

struct ft_list listen_socks;

///

struct stream_pair
{
	struct established_socket stream_in;
	struct established_socket stream_out;
};

struct ft_list stream_pairs; // List of established_socket(s) pairs


bool on_read_in_to_out(struct established_socket * established_sock, struct frame * frame)
{
	frame_flip(frame);
	struct stream_pair * pair = (struct stream_pair *)established_sock->data;
	bool ok = established_socket_write(&pair->stream_out, frame);
	if (!ok)
	{
		L_ERROR_P("Cannot write a frame!");
		frame_pool_return(frame);
	}

	return true;
}

bool on_read_out_to_in(struct established_socket * established_sock, struct frame * frame)
{
	frame_flip(frame);
	struct stream_pair * pair = (struct stream_pair *)established_sock->data;
	bool ok = established_socket_write(&pair->stream_in, frame);
	if (!ok)
	{
		L_ERROR_P("Cannot write a frame!");
		frame_pool_return(frame);
	}

	return true;
}

void on_connected(struct established_socket * established_sock)
{
	struct stream_pair * pair = (struct stream_pair *)established_sock->data;
	established_socket_read_start(&pair->stream_in);
	established_socket_read_start(&pair->stream_out);
}

void on_error(struct established_socket * established_sock)
{
	struct stream_pair * pair = (struct stream_pair *)established_sock->data;
	//TODO: maybe even hard shutdown
	established_socket_write_shutdown(&pair->stream_in);
	established_socket_write_shutdown(&pair->stream_out);
}

struct established_socket_cb sock_est_stream_in_cb = 
{
	.get_read_frame = established_socket_get_read_frame_simple,
	.read = on_read_in_to_out,
	.error = on_error,
};


struct established_socket_cb sock_est_stream_out_cb = 
{
	.get_read_frame = established_socket_get_read_frame_simple,
	.read = on_read_out_to_in,
	.connected = on_connected,
	.error = on_error,
};

static void established_sock_pair_stop_each(struct ft_node * node, void * data)
{
	struct stream_pair * pair = (struct stream_pair *)&node->data;
	established_socket_read_stop(&pair->stream_in);
	established_socket_write_stop(&pair->stream_in);
	established_socket_read_stop(&pair->stream_out);
	established_socket_write_stop(&pair->stream_out);
}

static void stream_pairs_on_remove(struct ft_list * list, struct ft_node * node)
{
	struct stream_pair * pair = (struct stream_pair *)&node->data;

	L_INFO("Stats  IN: Re:%u We:%u+%u Rb:%lu Wb:%lu",
		pair->stream_in.stats.read_events,
		pair->stream_in.stats.write_events,
		pair->stream_in.stats.write_direct,
		pair->stream_in.stats.read_bytes,
		pair->stream_in.stats.write_bytes
	);

	L_INFO("Stats OUT: Re:%u We:%u+%u Rb:%lu Wb:%lu",
		pair->stream_out.stats.read_events,
		pair->stream_out.stats.write_events,
		pair->stream_out.stats.write_direct,
		pair->stream_out.stats.read_bytes,
		pair->stream_out.stats.write_bytes
	);

	established_socket_fini(&pair->stream_in);
	established_socket_fini(&pair->stream_out);
}

///

static bool on_accept_cb(struct listening_socket * listening_socket, int fd, const struct sockaddr * client_addr, socklen_t client_addr_len)
{
	bool ok;

	struct ft_node * new_node = ft_node_new(sizeof(struct stream_pair));
	if (new_node == NULL) return false;

	struct stream_pair * pair = (struct stream_pair *)&new_node->data;

	// Initialize connection on the incoming connection
	ok = established_socket_init_accept(&pair->stream_in, &sock_est_stream_in_cb, listening_socket, fd, client_addr, client_addr_len);
	if (!ok)
	{
		ft_node_del(new_node);
		return false;
	}
	pair->stream_in.data = pair;
	established_socket_set_read_partial(&pair->stream_in, true);

	// Initialize connection on the outgoing connection
	ok = established_socket_init_connect(&pair->stream_out, &sock_est_stream_out_cb, listening_socket->context, target_addr);
	if (!ok)
	{
		ft_node_del(new_node);
		return false;
	}
	pair->stream_out.data = pair;
	established_socket_set_read_partial(&pair->stream_out, true);

	ok = established_socket_ssl_enable(&pair->stream_out, ssl_ctx);
	if (!ok)
	{
		ft_node_del(new_node);
		return false;
	}

	ft_list_add(&stream_pairs, new_node);

	return true;
}

struct listening_socket_cb sock_listen_sock_cb =
{
	.accept = on_accept_cb,
};

static bool on_resolve_listen_cb(void * data, struct context * context, struct addrinfo * addrinfo)
{
	struct ft_node * new_node = ft_node_new(sizeof(struct listening_socket));
	if (new_node == NULL) return false;

	bool ok = listening_socket_init((struct listening_socket *)new_node->data, &sock_listen_sock_cb, context, addrinfo);
	if (!ok) 
	{
		ft_node_del(new_node);
		return false;
	}

	ft_list_add(&listen_socks, new_node);

	return true;
}

static void listeners_on_remove(struct ft_list * list, struct ft_node * node)
{
	listening_socket_fini((struct listening_socket *)node->data);
}

static void listen_sock_start_each(struct ft_node * node, void * data)
{
	listening_socket_start((struct listening_socket *)node->data);
}

static void listen_sock_stop_each(struct ft_node * node, void * data)
{
	listening_socket_stop((struct listening_socket *)node->data);
}

///

static void on_exiting_cb(struct exiting_watcher * watcher, struct context * context)
{
	// Stop established sockets
	ft_list_each(&stream_pairs, established_sock_pair_stop_each, NULL);

	// Stop listening sockets
	ft_list_each(&listen_socks, listen_sock_stop_each, NULL);
}

static void on_check_cb(struct ev_loop * loop, ev_prepare * check, int revents)
{
restart:
	for (struct ft_node * node = stream_pairs.head; node != NULL; node = node->right)
	{
		struct stream_pair * pair = (struct stream_pair *)&node->data;
		if (established_socket_is_shutdown(&pair->stream_in) || established_socket_is_shutdown(&pair->stream_out))
		{
			ft_list_remove(&stream_pairs, node);
			goto restart;
		}
	}
}

///

struct addrinfo * resolve(const char * host, const char * port)
{
	int rc;
	struct addrinfo hints;

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = 0;
	hints.ai_protocol = 0;
	hints.ai_canonname = NULL;
	hints.ai_addr = NULL;
	hints.ai_next = NULL;

	struct addrinfo * res;

	rc = getaddrinfo(host, port, &hints, &res);
	if (rc != 0)
	{
		L_ERROR("getaddrinfo failed: %s", gai_strerror(rc));
		return NULL;
	}

	return res;
}

///

int main(int argc, char const *argv[])
{
	bool ok;
	int rc;
	struct context context;

	logging_set_verbose(true);
	libsccmn_init();

	//libsccmn_config.log_trace_mask |= L_TRACEID_SOCK_STREAM | L_TRACEID_EVENT_LOOP;

	// Initialize context
	ok = context_init(&context);
	if (!ok) return EXIT_FAILURE;

	// Initialize OpenSSL context
	ssl_ctx = SSL_CTX_new(SSLv23_client_method());
	if (ssl_ctx == NULL) return EXIT_FAILURE;

	ft_list_init(&listen_socks, listeners_on_remove);
	ft_list_init(&stream_pairs, stream_pairs_on_remove);

	// Prepare listening socket(s)
	rc = listening_socket_create_getaddrinfo(on_resolve_listen_cb, NULL, &context, AF_INET, SOCK_STREAM, "127.0.0.1", "12345");
	if (rc < 0) return EXIT_FAILURE;

	rc = listening_socket_create_getaddrinfo(on_resolve_listen_cb, NULL, &context, AF_UNIX, SOCK_STREAM, "/tmp/sctext.tmp", NULL);
	if (rc < 0) return EXIT_FAILURE;

	// Registed check handler
	ev_prepare prepare_w;
	ev_prepare_init(&prepare_w, on_check_cb);
	ev_prepare_start(context.ev_loop, &prepare_w);
	ev_unref(context.ev_loop);

	// Resolve target
	if (argc == 3)
	{
		L_INFO("Target set to %s %s", argv[1], argv[2]);
		target_addr = resolve( argv[1], argv[2]);
	}
	else
	{
		L_INFO("Target set to www.teskalabs.com 443");
		target_addr = resolve("www.teskalabs.com", "443");
	}
	if (target_addr == NULL)
	{
		L_ERROR("Cannot resolve target");
		return EXIT_FAILURE;
	}

	// Start listening
	ft_list_each(&listen_socks, listen_sock_start_each, NULL);

	// Register exiting watcher
	context_exiting_watcher_add(&context, &watcher, on_exiting_cb);

	// Enter event loop
	context_evloop_run(&context);

	// Remove established sockets
	ft_list_fini(&stream_pairs);

	// Remove listening sockets
	ft_list_fini(&listen_socks);

	// Finalize context
	context_fini(&context);

	return EXIT_SUCCESS;
}
