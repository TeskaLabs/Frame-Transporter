#include <stdlib.h>
#include <stdbool.h>

#include <libsccmn.h>

///

struct exiting_watcher watcher;
struct addrinfo * target_addr;

struct ft_list listeners;

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
		FT_ERROR_P("Cannot write a frame!");
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
		FT_ERROR_P("Cannot write a frame!");
		frame_pool_return(frame);
	}

	return true;
}

void on_error(struct established_socket * established_sock)
{
	struct stream_pair * pair = (struct stream_pair *)established_sock->data;
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

	established_socket_fini(&pair->stream_in);
	established_socket_fini(&pair->stream_out);
}

///

static bool on_accept_cb(struct listening_socket * listening_socket, int fd, const struct sockaddr * client_addr, socklen_t client_addr_len)
{
	bool ok;

	struct ft_list_node * new_node = ft_list_node_new(sizeof(struct stream_pair));
	if (new_node == NULL) return false;

	struct stream_pair * pair = (struct stream_pair *)&new_node->data;

	// Initialize connection on the incoming connection
	ok = established_socket_init_accept(&pair->stream_in, &stream_in_delegate, listening_socket, fd, client_addr, client_addr_len);
	if (!ok)
	{
		ft_list_node_del(new_node);
		return false;
	}
	pair->stream_in.data = pair;
	established_socket_set_read_partial(&pair->stream_in, true);

	// Initialize connection on the outgoing connection
	ok = established_socket_init_connect(&pair->stream_out, &stream_out_delegate, listening_socket->context, target_addr);
	if (!ok)
	{
		ft_list_node_del(new_node);
		return false;
	}
	pair->stream_out.data = pair;
	established_socket_set_read_partial(&pair->stream_out, true);

	ft_list_add(&stream_pairs, new_node);

	return true;
}

struct ft_listener_delegate listener_delegate =
{
	.accept = on_accept_cb,
};

///

static void on_exiting_cb(struct exiting_watcher * watcher, struct context * context)
{
	FT_LIST_FOR(&stream_pairs, node)
	{
		struct stream_pair * pair = (struct stream_pair *)&node->data;
		ft_stream_cntl(&pair->stream_in, FT_STREAM_READ_STOP | FT_STREAM_WRITE_STOP);
		ft_stream_cntl(&pair->stream_out, FT_STREAM_READ_STOP | FT_STREAM_WRITE_STOP);
	}

	ft_listener_list_cntl(&listeners, FT_LISTENER_STOP);
}

///

static void on_check_cb(struct ev_loop * loop, ev_prepare * check, int revents)
{
restart:
	FT_LIST_FOR(&stream_pairs, node)
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
	struct context context;

	ft_log_verbose(true);
	ft_initialise();
	
	//libsccmn_config.log_trace_mask |= L_TRACEID_SOCK_STREAM | L_TRACEID_EVENT_LOOP;

	// Initializa context
	ok = context_init(&context);
	if (!ok) return EXIT_FAILURE;

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
		FT_INFO("Target set to localhost 21");
		target_addr = resolve("localhost", "21");
	}
	if (target_addr == NULL)
	{
		FT_ERROR("Cannot resolve target");
		return EXIT_FAILURE;
	}

	// Start listening
	ft_listener_list_cntl(&listeners, FT_LISTENER_START);

	// Register exiting watcher
	context_exiting_watcher_add(&context, &watcher, on_exiting_cb);

	// Enter event loop
	context_evloop_run(&context);

	// Clean up
	ft_list_fini(&stream_pairs);
	ft_list_fini(&listeners);
	context_fini(&context);

	return EXIT_SUCCESS;
}
