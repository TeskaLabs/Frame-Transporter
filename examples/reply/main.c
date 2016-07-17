#include <stdlib.h>
#include <stdbool.h>

#include <ft.h>

///

struct context context;
struct exiting_watcher watcher;

struct ft_list listeners;
struct ft_list streams;

///

bool on_read(struct established_socket * established_sock, struct frame * frame)
{
//	frame_print(frame);	
	frame_flip(frame);
	bool ok = established_socket_write(established_sock, frame);
	if (!ok)
	{
		FT_ERROR("Cannot write a frame!");
		frame_pool_return(frame);
	}

	return true;
}

struct ft_stream_delegate stream_delegate = 
{
	.read = on_read,
};

static void streams_on_remove(struct ft_list * list, struct ft_list_node * node)
{
	struct established_socket * stream = (struct established_socket *)node->data;

	FT_INFO("Stats: Re:%u We:%u+%u Rb:%lu Wb:%lu",
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

	struct ft_list_node * new_node = ft_list_node_new(sizeof(struct established_socket));
	if (new_node == NULL) return false;

	struct established_socket * stream = (struct established_socket *)&new_node->data;

	ok = established_socket_init_accept(stream, &stream_delegate, listening_socket, fd, client_addr, client_addr_len);
	if (!ok)
	{
		ft_list_node_del(new_node);
		return false;
	}

	// Start read on the socket
	established_socket_set_read_partial(stream, true);

	ft_list_add(&streams, new_node);

	return true;
}

struct ft_listener_delegate listener_delegate =
{
	.accept = on_accept_cb,
};

///

static void on_exiting_cb(struct exiting_watcher * watcher, struct context * context)
{
	FT_LIST_FOR(&streams, node)
	{
		struct established_socket * stream = (struct established_socket *)node->data;
		ft_stream_cntl(stream, FT_STREAM_READ_STOP | FT_STREAM_WRITE_STOP);
	}

	ft_listener_list_cntl(&listeners, FT_LISTENER_STOP);
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
	FT_LIST_FOR(&streams, node)
	{
		struct established_socket * sock = (struct established_socket *)node->data;

		if (established_socket_is_shutdown(sock))
		{
			ft_list_remove(&streams, node);
			goto restart; // List has been changed during iteration
		}

		if ((throttle) && (sock->flags.read_throttle == false))
		{
			ft_stream_cntl(sock, FT_STREAM_READ_PAUSE);
		}
		else if ((!throttle) && (sock->flags.read_throttle == true))
		{
			ft_stream_cntl(sock, FT_STREAM_READ_RESUME);
		}
	}
}

///

int main(int argc, char const *argv[])
{
	bool ok;
	int rc;

	ft_log_verbose(true);
	//ft_config.log_trace_mask |= FT_TRACE_ID_SOCK_STREAM | FT_TRACE_ID_EVENT_LOOP;

	ft_initialise();
	
	// Initializa context
	ok = context_init(&context);
	if (!ok) return EXIT_FAILURE;

#ifdef MAP_HUGETLB
	FT_INFO("Using hugetlb pages!");
	frame_pool_set_alloc_advise(&context.frame_pool, frame_pool_zone_alloc_advice_hugetlb);
#endif

	// Initialize a list for listening sockets
	ft_list_init(&streams, streams_on_remove);

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

	// Start listening
	ft_listener_list_cntl(&listeners, FT_LISTENER_START);

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
