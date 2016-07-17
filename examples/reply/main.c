#include <stdlib.h>
#include <stdbool.h>

#include <ft.h>

///

struct ft_context context;
struct ft_list listeners;
struct ft_list streams;

///

bool on_read(struct ft_stream * established_sock, struct ft_frame * frame)
{
//	ft_frame_fprintf(stdout, frame);	
	ft_frame_flip(frame);
	bool ok = ft_stream_write(established_sock, frame);
	if (!ok)
	{
		FT_ERROR("Cannot write a frame!");
		ft_frame_return(frame);
	}

	return true;
}

struct ft_stream_delegate stream_delegate = 
{
	.read = on_read,
};

static void streams_on_remove(struct ft_list * list, struct ft_list_node * node)
{
	struct ft_stream * stream = (struct ft_stream *)node->data;

	FT_INFO("Stats: Re:%u We:%u+%u Rb:%lu Wb:%lu",
		stream->stats.read_events,
		stream->stats.write_events,
		stream->stats.write_direct,
		stream->stats.read_bytes,
		stream->stats.write_bytes
	);

	ft_stream_fini(stream);	
}

///

static bool on_accept_cb(struct ft_listener * listening_socket, int fd, const struct sockaddr * client_addr, socklen_t client_addr_len)
{
	bool ok;

	struct ft_list_node * new_node = ft_list_node_new(sizeof(struct ft_stream));
	if (new_node == NULL) return false;

	struct ft_stream * stream = (struct ft_stream *)&new_node->data;

	ok = ft_stream_accept(stream, &stream_delegate, listening_socket, fd, client_addr, client_addr_len);
	if (!ok)
	{
		ft_list_node_del(new_node);
		return false;
	}

	// Start read on the socket
	ft_stream_set_partial(stream, true);

	ft_list_add(&streams, new_node);

	return true;
}

struct ft_listener_delegate listener_delegate =
{
	.accept = on_accept_cb,
};

///

static void on_termination_cb(struct ft_context * context, void * data)
{
	FT_LIST_FOR(&streams, node)
	{
		struct ft_stream * stream = (struct ft_stream *)node->data;
		ft_stream_cntl(stream, FT_STREAM_READ_STOP | FT_STREAM_WRITE_STOP);
	}

	ft_listener_list_cntl(&listeners, FT_LISTENER_STOP);
}


static void on_check_cb(struct ev_loop * loop, ev_prepare * check, int revents)
{
	size_t avail = ft_pool_count_available_frames(&context.frame_pool);
	bool throttle = avail < 8;

	if (throttle)
	{
		// We can still allocated new zone
		if (ft_pool_count_zones(&context.frame_pool) < 2) throttle = false;
	}

	// Check all established socket and remove closed ones
restart:
	FT_LIST_FOR(&streams, node)
	{
		struct ft_stream * sock = (struct ft_stream *)node->data;

		if (ft_stream_is_shutdown(sock))
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
	//ft_config.log_trace_mask |= FT_TRACE_ID_STREAM | FT_TRACE_ID_EVENT_LOOP;

	ft_initialise();
	
	// Initializa context
	ok = ft_context_init(&context);
	if (!ok) return EXIT_FAILURE;

	ft_context_at_termination(&context, on_termination_cb, NULL);

#ifdef MAP_HUGETLB
	FT_INFO("Using hugetlb pages!");
	ft_pool_set_alloc(&context.frame_pool, ft_pool_alloc_hugetlb);
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

	// Enter event loop
	ft_context_run(&context);

	// Clean up
	ft_list_fini(&streams);
	ft_list_fini(&listeners);
	ft_context_fini(&context);

	return EXIT_SUCCESS;
}
