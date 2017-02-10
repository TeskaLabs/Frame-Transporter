#include <stdlib.h>
#include <stdbool.h>

#include <ft.h>

///

struct addrinfo * target_addr;
SSL_CTX * ssl_ctx;

struct ft_list listeners;

///

struct stream_pair
{
	struct ft_stream stream_in;
	struct ft_stream stream_out;
};

struct ft_list stream_pairs; // List of established_socket(s) pairs


bool on_read_in_to_out(struct ft_stream * established_sock, struct ft_frame * frame)
{
	ft_frame_flip(frame);
	struct stream_pair * pair = (struct stream_pair *)established_sock->base.socket.data;
	bool ok = ft_stream_write(&pair->stream_out, frame);
	if (!ok)
	{
		FT_ERROR_P("Cannot write a frame!");
		ft_frame_return(frame);
	}

	return true;
}

bool on_read_out_to_in(struct ft_stream * established_sock, struct ft_frame * frame)
{
	ft_frame_flip(frame);
	struct stream_pair * pair = (struct stream_pair *)established_sock->base.socket.data;
	bool ok = ft_stream_write(&pair->stream_in, frame);
	if (!ok)
	{
		FT_ERROR_P("Cannot write a frame!");
		ft_frame_return(frame);
	}

	return true;
}

void on_error(struct ft_stream * established_sock)
{
	struct stream_pair * pair = (struct stream_pair *)established_sock->base.socket.data;
	//TODO: maybe even hard shutdown
	ft_stream_cntl(&pair->stream_in, FT_STREAM_WRITE_SHUTDOWN);
	ft_stream_cntl(&pair->stream_out, FT_STREAM_WRITE_SHUTDOWN);
}

struct ft_stream_delegate stream_in_delegate = 
{
	.read = on_read_in_to_out,
	.error = on_error,
};


struct ft_stream_delegate stream_out_delegate = 
{
	.read = on_read_out_to_in,
	.error = on_error,
};

static void stream_pairs_on_remove(struct ft_list * list, struct ft_list_node * node)
{
	struct stream_pair * pair = (struct stream_pair *)&node->data;

	FT_INFO("Stats  IN: Re:%u We:%u+%u Rb:%lu Wb:%lu",
		pair->stream_in.stats.read_events,
		pair->stream_in.stats.write_events,
		pair->stream_in.stats.write_direct,
		pair->stream_in.stats.read_bytes,
		pair->stream_in.stats.write_bytes
	);

	FT_INFO("Stats OUT: Re:%u We:%u+%u Rb:%lu Wb:%lu",
		pair->stream_out.stats.read_events,
		pair->stream_out.stats.write_events,
		pair->stream_out.stats.write_direct,
		pair->stream_out.stats.read_bytes,
		pair->stream_out.stats.write_bytes
	);

	ft_stream_fini(&pair->stream_in);
	ft_stream_fini(&pair->stream_out);
}

static int on_ssl_cert_verify_callback(X509_STORE_CTX *ctx, void *arg)
{
    return 1;
}

///

static bool on_accept_cb(struct ft_listener * listening_socket, int fd, const struct sockaddr * client_addr, socklen_t client_addr_len)
{
	bool ok;

	struct ft_list_node * new_node = ft_list_node_new(sizeof(struct stream_pair));
	if (new_node == NULL) return false;

	struct stream_pair * pair = (struct stream_pair *)&new_node->data;

	// Initialize connection on the incoming connection
	ok = ft_stream_accept(&pair->stream_in, &stream_in_delegate, listening_socket, fd, client_addr, client_addr_len);
	if (!ok)
	{
		ft_list_node_del(new_node);
		return false;
	}
	pair->stream_in.base.socket.data = pair;
	ft_stream_set_partial(&pair->stream_in, true);

	// Initialize connection on the outgoing connection
	ok = ft_stream_connect(&pair->stream_out, &stream_out_delegate, listening_socket->base.socket.context, target_addr);
	if (!ok)
	{
		ft_list_node_del(new_node);
		return false;
	}
	pair->stream_out.base.socket.data = pair;
	ft_stream_set_partial(&pair->stream_out, true);

	ok = ft_stream_enable_ssl(&pair->stream_out, ssl_ctx);
	if (!ok)
	{
		ft_list_node_del(new_node);
		return false;
	}

	ft_list_add(&stream_pairs, new_node);

	return true;
}

struct ft_listener_delegate listener_delegate =
{
	.accept = on_accept_cb,
};

///

static void on_termination_cb(struct ft_context * context, void * data)
{
	FT_LIST_FOR(&stream_pairs, node)
	{
		struct stream_pair * pair = (struct stream_pair *)&node->data;
		ft_stream_cntl(&pair->stream_in, FT_STREAM_READ_STOP | FT_STREAM_WRITE_STOP);
		ft_stream_cntl(&pair->stream_out, FT_STREAM_READ_STOP | FT_STREAM_WRITE_STOP);
	}

	ft_listener_list_cntl(&listeners, FT_LISTENER_STOP);
}

static void on_check_cb(struct ev_loop * loop, ev_prepare * check, int revents)
{
restart:
	FT_LIST_FOR(&stream_pairs, node)
	{
		struct stream_pair * pair = (struct stream_pair *)&node->data;
		if (ft_stream_is_shutdown(&pair->stream_in) || ft_stream_is_shutdown(&pair->stream_out))
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
		FT_ERROR("getaddrinfo failed: %s", gai_strerror(rc));
		return NULL;
	}

	return res;
}

///

int main(int argc, char const *argv[])
{
	bool ok;
	int rc;
	struct ft_context context;

	ft_config.log_verbose = true;
	ft_initialise();

	//ft_config.log_trace_mask |= FT_TRACE_ID_STREAM | FT_TRACE_ID_EVENT_LOOP;

	// Initialize context
	ok = ft_context_init(&context);
	if (!ok) return EXIT_FAILURE;

	ft_context_at_termination(&context, on_termination_cb, NULL);

	// Initialize OpenSSL context
	ssl_ctx = SSL_CTX_new(SSLv23_client_method());
	if (ssl_ctx == NULL) return EXIT_FAILURE;

	SSL_CTX_set_cert_verify_callback(ssl_ctx, on_ssl_cert_verify_callback, NULL);

	ft_list_init(&stream_pairs, stream_pairs_on_remove);

	// Prepare listening socket(s)
	ft_listener_list_init(&listeners);

	rc = ft_listener_list_extend(&listeners, &listener_delegate, &context, AF_INET, SOCK_STREAM, "127.0.0.1", "12345");
	if (rc < 0) return EXIT_FAILURE;

	rc = ft_listener_list_extend(&listeners, &listener_delegate, &context, AF_UNIX, SOCK_STREAM, "/tmp/sctext.tmp", NULL);
	if (rc < 0) return EXIT_FAILURE;

	// Registed check handler
	ev_prepare prepare_w;
	ev_prepare_init(&prepare_w, on_check_cb);
	ev_prepare_start(context.ev_loop, &prepare_w);
	ev_unref(context.ev_loop);

	// Resolve target
	if (argc == 3)
	{
		FT_INFO("Target set to %s %s", argv[1], argv[2]);
		target_addr = resolve( argv[1], argv[2]);
	}
	else
	{
		FT_INFO("Target set to www.teskalabs.com 443");
		target_addr = resolve("www.teskalabs.com", "443");
	}
	if (target_addr == NULL)
	{
		FT_ERROR("Cannot resolve target");
		return EXIT_FAILURE;
	}

	// Start listening
	ft_listener_list_cntl(&listeners, FT_LISTENER_START);

	// Enter event loop
	ft_context_run(&context);

	// Clean up
	ft_list_fini(&stream_pairs);
	ft_list_fini(&listeners);
	ft_context_fini(&context);

	return EXIT_SUCCESS;
}
