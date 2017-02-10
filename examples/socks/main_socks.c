#include <stdlib.h>
#include <stdbool.h>

#include <ft.h>

struct ft_list listeners;
struct ft_list sock_relays;

struct sock_relay
{
	struct ft_stream in;
	struct ft_stream out;

	struct ft_proto_socks proto_socks;
};

extern struct ft_proto_socks_delegate sock_relay_sock_in_delegate;
extern struct ft_stream_delegate sock_relay_in_delegate;
extern struct ft_stream_delegate sock_relay_out_delegate;

bool sock_relay_init(struct sock_relay * this, struct ft_listener * listening_socket, int fd, const struct sockaddr * client_addr, socklen_t client_addr_len)
{
	bool ok;

	assert(this != NULL);

	// Initialize connection on the incoming connection
	ok = ft_stream_accept(&this->in, &ft_stream_proto_socks_delegate, listening_socket, fd, client_addr, client_addr_len);
	if (!ok) return false;
	
	this->in.base.socket.data = this;

	ok = ft_proto_socks_init(&this->proto_socks, &sock_relay_sock_in_delegate, &this->in);
	if (!ok)
	{
		ft_stream_fini(&this->in);
		return false;
	}

	return true;
}

//

void on_error(struct ft_stream * established_sock)
{
	struct sock_relay * relay = (struct sock_relay *)established_sock->base.socket.data;
	ft_stream_cntl(&relay->in, FT_STREAM_WRITE_SHUTDOWN);
	if (relay->out.base.socket.clazz != NULL)
		ft_stream_cntl(&relay->out, FT_STREAM_WRITE_SHUTDOWN);
}

//

static bool sock_relay_on_sock_connect(struct ft_stream * stream, char addrtype, char * host, unsigned int port)
{
	bool ok;
	int rc;

	struct sock_relay * this = (struct sock_relay *)stream->base.socket.data;
	assert(this != NULL);

	struct addrinfo hints;

	memset(&hints, 0, sizeof(struct addrinfo));
	
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = 0;
	hints.ai_protocol = 0;
	hints.ai_canonname = NULL;
	hints.ai_addr = NULL;
	hints.ai_next = NULL;

	switch (addrtype)
	{
		case '4':
			hints.ai_family = AF_INET;
			break;

		case '6':
			hints.ai_family = AF_INET6;
			break;

		case 'D':
		default:
			hints.ai_family = AF_UNSPEC;
			break;
	}

	struct addrinfo * addr;

	char portstr[16];
	snprintf(portstr, sizeof(portstr)-1, "%u", port);

	rc = getaddrinfo(host, portstr, &hints, &addr);
	if (rc != 0)
	{
		FT_ERROR("getaddrinfo failed: %s (h:'%s' p:'%u')", gai_strerror(rc), host, port);
		return false;
	}

	// Initialize connection on the outgoing connection
	ok = ft_stream_connect(&this->out, &sock_relay_out_delegate, stream->base.socket.context, addr);
	freeaddrinfo(addr);
	if (!ok)
	{
		return false;
	}
	this->out.base.socket.data = this;
	ft_stream_set_partial(&this->out, true);

	return true;
}

void sock_relay_on_out_connected(struct ft_stream * stream)
{
	struct sock_relay * this = (struct sock_relay *)stream->base.socket.data;
	assert(this != NULL);

	FT_INFO("SOCK relay connected to target");

	ft_proto_socks_stream_send_final_response(&this->in, 0); // Inform SOCKS about success

	this->in.delegate = &sock_relay_in_delegate;
	ft_stream_set_partial(&this->in, true);

	ft_proto_socks_fini(&this->proto_socks);
}

struct ft_proto_socks_delegate sock_relay_sock_in_delegate = {
	.connect = sock_relay_on_sock_connect,
	.error = on_error
};

///

bool on_read_in_to_out(struct ft_stream * established_sock, struct ft_frame * frame)
{
	ft_frame_flip(frame);

	struct sock_relay * relay = (struct sock_relay *)established_sock->base.socket.data;
	bool ok = ft_stream_write(&relay->out, frame);
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
	struct sock_relay * relay = (struct sock_relay *)established_sock->base.socket.data;
	bool ok = ft_stream_write(&relay->in, frame);
	if (!ok)
	{
		FT_ERROR_P("Cannot write a frame!");
		ft_frame_return(frame);
	}

	return true;
}

struct ft_stream_delegate sock_relay_in_delegate = 
{
	.read = on_read_in_to_out,
	.error = on_error,
};

struct ft_stream_delegate sock_relay_out_delegate = 
{
	.connected = sock_relay_on_out_connected,
	.read = on_read_out_to_in,
	.error = on_error,
};

static void sock_relays_on_remove(struct ft_list * list, struct ft_list_node * node)
{
	struct sock_relay * relay = (struct sock_relay *)&node->data;

	FT_INFO("Stats  IN: Re:%u We:%u+%u Rb:%lu Wb:%lu",
		relay->in.stats.read_events,
		relay->in.stats.write_events,
		relay->in.stats.write_direct,
		relay->in.stats.read_bytes,
		relay->in.stats.write_bytes
	);

	if (relay->out.base.socket.clazz != NULL)
		FT_INFO("Stats OUT: Re:%u We:%u+%u Rb:%lu Wb:%lu",
			relay->out.stats.read_events,
			relay->out.stats.write_events,
			relay->out.stats.write_direct,
			relay->out.stats.read_bytes,
			relay->out.stats.write_bytes
		);

	ft_stream_fini(&relay->in);
	if (relay->out.base.socket.clazz != NULL)
		ft_stream_fini(&relay->out);
}

///

static bool on_accept_cb(struct ft_listener * listening_socket, int fd, const struct sockaddr * client_addr, socklen_t client_addr_len)
{
	bool ok;

	struct ft_list_node * new_node = ft_list_node_new(sizeof(struct sock_relay));
	if (new_node == NULL) return false;

	struct sock_relay * sock_relay = (struct sock_relay *)&new_node->data;
	ok = sock_relay_init(sock_relay, listening_socket, fd, client_addr, client_addr_len);
	if (!ok)
	{
		ft_list_node_del(new_node);
		return false;
	}

	ft_list_add(&sock_relays, new_node);

	FT_INFO("New SOCKS relay");

	return true;
}

struct ft_listener_delegate listener_delegate =
{
	.accept = on_accept_cb,
};

///

static void on_termination_cb(struct ft_context * context, void * data)
{
	FT_LIST_FOR(&sock_relays, node)
	{
		struct sock_relay * relay = (struct sock_relay *)&node->data;
		ft_stream_cntl(&relay->in, FT_STREAM_READ_STOP | FT_STREAM_WRITE_STOP);
		if (relay->out.base.socket.clazz != NULL)
			ft_stream_cntl(&relay->out, FT_STREAM_READ_STOP | FT_STREAM_WRITE_STOP);
	}

	ft_listener_list_cntl(&listeners, FT_LISTENER_STOP);
}

///

static void on_check_cb(struct ev_loop * loop, ev_prepare * check, int revents)
{
restart:
	FT_LIST_FOR(&sock_relays, node)
	{
		struct sock_relay * relay = (struct sock_relay *)&node->data;
		if (ft_stream_is_shutdown(&relay->in) || ft_stream_is_shutdown(&relay->out))
		{
			ft_list_remove(&sock_relays, node);
			goto restart;
		}
	}
}

///

int main(int argc, char const *argv[])
{
	bool ok;
	int rc;
	struct ft_context context;

	//ft_config.log_verbose = true;
	//ft_config.log_trace_mask |= FT_TRACE_ID_STREAM | FT_TRACE_ID_EVENT_LOOP | FT_TRACE_ID_PROTO_SOCK;

	ft_initialise();
	
	// Initializa context
	ok = ft_context_init(&context);
	if (!ok) return EXIT_FAILURE;

	ft_context_at_termination(&context, on_termination_cb, NULL);

	ft_list_init(&sock_relays, sock_relays_on_remove);

	// Prepare listening socket(s)
	ft_listener_list_init(&listeners);

	rc = ft_listener_list_extend(&listeners, &listener_delegate, &context, AF_INET, SOCK_STREAM, "127.0.0.1", "12345");
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
	ft_list_fini(&sock_relays);
	ft_list_fini(&listeners);
	ft_context_fini(&context);

	return EXIT_SUCCESS;
}


