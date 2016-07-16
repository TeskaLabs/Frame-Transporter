#include <stdlib.h>
#include <stdbool.h>

#include <libsccmn.h>

///

SSL_CTX * ssl_ctx;
struct context context;
struct exiting_watcher watcher;

struct ft_list streams;
struct ft_list listeners;

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
	.get_read_frame = established_socket_get_read_frame_simple,
	.read = on_read,
	.error = NULL,
};

static void established_sock_stop_each(struct ft_node * node, void * data)
{
	struct established_socket * stream = (struct established_socket *)node->data;
	established_socket_read_stop(stream);
	established_socket_write_stop(stream);
}

static void streams_on_remove(struct ft_list * list, struct ft_node * node)
{
	struct established_socket * stream = (struct established_socket *)node->data;

	L_INFO("Stats: Re:%u We:%u+%u Rb:%lu Wb:%lu",
		stream->stats.read_events,
		stream->stats.write_events,
		stream->stats.write_direct,
		stream->stats.read_bytes,
		stream->stats.write_bytes
	);

	established_socket_fini(stream);
}

///

static bool on_accept_cb(struct listening_socket * listening_socket, int fd, const struct sockaddr * client_addr, socklen_t client_addr_len)
{
	bool ok;

	struct ft_node * new_node = ft_node_new(sizeof(struct established_socket));
	if (new_node == NULL) return false;

	struct established_socket * established_sock = (struct established_socket *)&new_node->data;

	ok = established_socket_init_accept(established_sock, &sock_est_sock_cb, listening_socket, fd, client_addr, client_addr_len);
	if (!ok)
	{
		ft_node_del(new_node);
		return false;
	}

	established_socket_set_read_partial(established_sock, true);

	// Initiate SSL
	ok = established_socket_ssl_enable(established_sock, ssl_ctx);
	if (!ok) return EXIT_FAILURE;


	ft_list_add(&streams, new_node);

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

	ft_list_add(&listeners, new_node);

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
	ft_list_each(&streams, established_sock_stop_each, NULL);

	// Stop listening sockets
	ft_list_each(&listeners, listen_sock_stop_each, NULL);
}

static void on_check_cb(struct ev_loop * loop, ev_prepare * check, int revents)
{
	size_t avail = frame_pool_available_frames_count(&context.frame_pool);
	bool throttle = avail < 8;

	if (throttle)
	{
		// We can still allocated new zone
		if (frame_pool_zones_count(&context.frame_pool) < 2) throttle = false;
	}

	// Check all established socket and remove closed ones
restart:
	for (struct ft_node * node = streams.head; node != NULL; node = node->right)
	{
		struct established_socket * sock = (struct established_socket *)node->data;

		if (established_socket_is_shutdown(sock))
		{
			ft_list_remove(&streams, node);
			goto restart;
		}

		if (((!throttle) && (sock->flags.read_throttle == true)) || ((throttle) && (sock->flags.read_throttle == false)))
		{
			established_socket_read_throttle(sock, throttle);
		}
	}
}

int main(int argc, char const *argv[])
{
	bool ok;
	int rc;

	//logging_set_verbose(true);
	//libsccmn_config.log_trace_mask |= L_TRACEID_SOCK_STREAM | L_TRACEID_EVENT_LOOP;

	libsccmn_init();
	
	// Initializa context
	ok = context_init(&context);
	if (!ok) return EXIT_FAILURE;

	// Initialize OpenSSL context
	ssl_ctx = SSL_CTX_new(SSLv23_server_method());
	if (ssl_ctx == NULL) return EXIT_FAILURE;

	long ssloptions = SSL_OP_ALL | SSL_OP_NO_SESSION_RESUMPTION_ON_RENEGOTIATION | SSL_OP_NO_COMPRESSION
	| SSL_OP_CIPHER_SERVER_PREFERENCE | SSL_OP_SINGLE_DH_USE;
	SSL_CTX_set_options(ssl_ctx, ssloptions);

	rc = SSL_CTX_use_PrivateKey_file(ssl_ctx, "./key.pem", SSL_FILETYPE_PEM);
	if (rc != 1) return EXIT_FAILURE;

	rc = SSL_CTX_use_certificate_file(ssl_ctx, "./cert.pem", SSL_FILETYPE_PEM);
	if (rc != 1) return EXIT_FAILURE;

	ft_list_init(&listeners, listeners_on_remove);
	ft_list_init(&streams, streams_on_remove);

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

	// Start listening
	ft_list_each(&listeners, listen_sock_start_each, NULL);

	// Register exiting watcher
	context_exiting_watcher_add(&context, &watcher, on_exiting_cb);

	// Enter event loop
	context_evloop_run(&context);

	// Clean up
	ft_list_fini(&streams);
	ft_list_fini(&listeners);
	context_fini(&context);

	return EXIT_SUCCESS;
}
