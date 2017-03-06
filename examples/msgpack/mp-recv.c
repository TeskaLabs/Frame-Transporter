#include <stdlib.h>
#include <stdbool.h>

#include <ft.h>
#include <msgpack.h>

///

struct ft_context context;
struct ft_list listeners;
struct ft_list streams;

///

bool on_read(struct ft_stream * established_sock, struct ft_frame * frame)
{
	if (frame->type == FT_FRAME_TYPE_END_OF_STREAM)
	{
		ft_stream_write(established_sock, frame);
		return true;
	}

	ft_frame_flip(frame);

	struct ft_vec * vec = ft_frame_get_vec(frame);
	assert(vec != NULL);


	msgpack_zone mempool;
    msgpack_zone_init(&mempool, 2048);

    msgpack_object deserialized;
    msgpack_unpack(ft_vec_ptr(vec), ft_vec_len(vec), NULL, &mempool, &deserialized);

    /* print the deserialized object. */
    msgpack_object_print(stdout, deserialized);
    puts("");

    msgpack_zone_destroy(&mempool);

	ft_frame_return(frame);
	return true;
}

struct ft_stream_delegate stream_delegate = 
{
	.read = on_read,
};

static void streams_on_remove(struct ft_list * list, struct ft_list_node * node)
{
	struct ft_stream * stream = (struct ft_stream *)node->data;
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

static void on_exit_cb(struct ft_subscriber * subscriber, struct ft_pubsub * pubsub, const char * topic, void * data)
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

	ft_config.log_verbose = true;
	//ft_config.log_trace_mask |= FT_TRACE_ID_STREAM | FT_TRACE_ID_EVENT_LOOP;

	ft_initialise();
	
	// Initializa context
	ok = ft_context_init(&context);
	if (!ok) return EXIT_FAILURE;

	struct ft_subscriber exit;
	ft_subscriber_init(&exit, on_exit_cb);
	ft_subscriber_subscribe(&exit, &context.pubsub, FT_PUBSUB_TOPIC_EXIT);

#ifdef MAP_HUGETLB
	FT_INFO("Using hugetlb pages!");
	ft_pool_set_alloc(&context.frame_pool, ft_pool_alloc_hugetlb);
#endif

	// Initialize a list for listening sockets
	ft_list_init(&streams, streams_on_remove);

	// Prepare listening socket(s)
	ft_listener_list_init(&listeners);

	rc = ft_listener_list_extend(&listeners, &listener_delegate, &context, AF_UNIX, SOCK_STREAM, "/tmp/scpb.tmp", NULL);
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
