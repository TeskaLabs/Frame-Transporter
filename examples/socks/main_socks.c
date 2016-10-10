#include <stdlib.h>
#include <stdbool.h>

#include <ft.h>

/*
See:

http://ftp.icm.edu.pl/packages/socks/socks4/SOCKS4.protocol
https://www.openssh.com/txt/socks4a.protocol

*/

struct addrinfo * resolve(const char * host, const char * port);

struct ft_list listeners;
struct ft_list sock_relays;

struct sock_relay
{
	struct ft_stream in;
	struct ft_stream out;

	//SOCK4 headers
	uint8_t VN;
	uint8_t CD;
	uint16_t DSTPORT;
	uint32_t DSTIP;
};

extern struct ft_stream_delegate sock_relay_in_prologue_delegate;
extern struct ft_stream_delegate sock_relay_in_delegate;
extern struct ft_stream_delegate sock_relay_out_delegate;

bool sock_relay_init(struct sock_relay * this, struct ft_listener * listening_socket, int fd, const struct sockaddr * client_addr, socklen_t client_addr_len)
{
	bool ok;

	assert(this != NULL);

	this->VN = 0;
	this->CD = 0;
	this->DSTPORT = 0;
	this->DSTIP = 0;

	// Initialize connection on the incoming connection
	ok = ft_stream_accept(&this->in, &sock_relay_in_prologue_delegate, listening_socket, fd, client_addr, client_addr_len);
	if (!ok)
	{
		return false;
	}
	this->in.base.socket.data = this;

	return true;
}


static struct ft_frame * sock_relay_in_prologue_on_get_read_frame(struct ft_stream * stream)
{
	struct ft_frame * frame = ft_pool_borrow(&stream->base.socket.context->frame_pool, FT_FRAME_TYPE_RAW_DATA);
	if (frame == NULL) return NULL;

	ft_frame_format_empty(frame);
	ft_frame_create_vec(frame, 0, 1); // Parse  a VN

	return frame;
}


static void sock_relay_on_socks4_request(struct sock_relay * this, struct ft_stream * stream, struct ft_frame * frame)
{
	bool ok;
	struct addrinfo * target_addr = NULL;

	assert(this != NULL);

	if (this->CD != 1)
	{
		FT_ERROR_P("Invalid SOCKS command code (CD: %u)", this->CD);
		goto exit_send_error_response;
	}

	if ((this->VN == 4) && (this->DSTIP > 0) && (this->DSTIP <= 255))
	{
		// SOCK4A
		char portstr[16];
		snprintf(portstr, sizeof(portstr)-1, "%u", this->DSTPORT);
		char * hoststr = ft_vec_ptr(ft_frame_get_vec_at(frame, 3));

		target_addr = resolve(hoststr, portstr);
	}

	else if (this->VN == 4)
	{
		// SOCK4
		char portstr[16];
		snprintf(portstr, sizeof(portstr)-1, "%u", this->DSTPORT);

		char addrstr[32];
		snprintf(addrstr, sizeof(addrstr), "%u.%u.%u.%u", this->DSTIP >> 24 & 0xFF, this->DSTIP >> 16 & 0xFF, this->DSTIP >> 8 & 0xFF, this->DSTIP & 0xFF);

		target_addr = resolve(addrstr, portstr);
	}

	else
	{
		FT_ERROR_P("Unknown SOCKS protocol");
		goto exit_send_error_response;
	}

	if (target_addr == NULL)
	{
		FT_ERROR("Cannot resolve target host");
		goto exit_send_error_response;
	}

	// Initialize connection on the outgoing connection
	ok = ft_stream_connect(&this->out, &sock_relay_out_delegate, stream->base.socket.context, target_addr);
	freeaddrinfo(target_addr);
	if (!ok)
	{
		goto exit_send_error_response;
	}
	this->out.base.socket.data = this;
	ft_stream_set_partial(&this->out, true);	

	ft_frame_return(frame);

	this->in.delegate = &sock_relay_in_delegate;
	ft_stream_set_partial(&this->in, true);

	return;

exit_send_error_response:

	// Prepare response frame
	ft_frame_format_simple(frame);
	struct ft_vec * vec = ft_frame_get_vec(frame);
	uint8_t * cursor = ft_vec_ptr(vec);
	cursor = ft_store_u8(cursor, 0);  // VN
	cursor = ft_store_u8(cursor, 91); // CD
	cursor = ft_store_u16(cursor, 0); // DSTPORT
	cursor = ft_store_u32(cursor, 0); // DSTIP

	vec->limit = 8;
	vec->position = 8;

	ft_frame_flip(frame);

	ft_stream_write(stream, frame);
	ft_stream_cntl(&this->in, FT_STREAM_WRITE_SHUTDOWN);

	return;
}

static void sock_relay_on_sock_connected(struct ft_stream * stream)
{
	struct sock_relay * this = (struct sock_relay *)stream->base.socket.data;
	assert(this != NULL);

	FT_INFO("SOCK relay connected to target");

	struct ft_frame * frame = ft_pool_borrow(&stream->base.socket.context->frame_pool, FT_FRAME_TYPE_RAW_DATA);

	ft_frame_format_simple(frame);
	struct ft_vec * vec = ft_frame_get_vec(frame);
	uint8_t * cursor = ft_vec_ptr(vec);
	cursor = ft_store_u8(cursor, 0);  // VN
	cursor = ft_store_u8(cursor, 90); // CD
	cursor = ft_store_u16(cursor, 0); // DSTPORT (value is ignored based on protocol specs)
	cursor = ft_store_u32(cursor, 0); // DSTIP (value is ignored based on protocol specs)

	vec->limit = 8;
	vec->position = 8;

	ft_frame_flip(frame);

	ft_stream_write(&this->in, frame);
}


static bool sock_relay_in_prologue_on_read(struct ft_stream * established_sock, struct ft_frame * frame)
{
	// This function basically implements parser for SOCK4 and SOCK4A protocol
	struct sock_relay * this = (struct sock_relay *)established_sock->base.socket.data;
	assert(this != NULL);

	uint8_t * cursor;
	struct ft_vec * vec;

	switch (frame->vec_limit)
	{
		case 1:
			// Parsing a VN
			assert(this->VN == 0);
			cursor = ft_vec_begin_ptr(ft_frame_get_vec_at(frame, 0));
			cursor = ft_load_u8(cursor, &this->VN);

			if (this->VN == 4) // SOCK4
			{
				ft_frame_create_vec(frame, 0, 7); // Read additional 7 bytes to the next vec
				return false; //Continue reading
			}
		
			FT_ERROR("Unknown SOCKS protocol version (VN: %u, vl:%d)", this->VN, frame->vec_limit);
			break;

		case 2:
			if (this->VN == 4) // SOCK4
			{
				cursor = ft_vec_begin_ptr(ft_frame_get_vec_at(frame, 1));
				cursor = ft_load_u8(cursor, &this->CD);
				cursor = ft_load_u16(cursor, &this->DSTPORT);
				cursor = ft_load_u32(cursor, &this->DSTIP);

				ft_frame_create_vec(frame, 0, 1);
				return false; //Continue reading
			}
			
			FT_ERROR("Unknown SOCKS protocol version (VN: %u, vl: %d)", this->VN, frame->vec_limit);
			break;


		case 3:
			if (this->VN == 4) // SOCK4
			{
				// Parse USERID

				vec = ft_frame_get_vec_at(frame, 2);
				char * p = ft_vec_ptr(vec);	

				if (p[-1] == '\0')
				{
					if ((this->DSTIP > 0) && (this->DSTIP <= 255)) // SOCK4A
					{
						ft_frame_create_vec(frame, 0, 1);
						return false; //Continue reading
					}

					else
					{
						sock_relay_on_socks4_request(this, established_sock, frame);
						return true;
					}
				}

				// TODO: Ensure that capacity is sane
				// Keep filling this frame ...
				ft_frame_prev_vec(frame);
				vec->capacity += 1;
				vec->limit = vec->capacity;
				return false;
			}

			FT_ERROR("Unknown SOCKS protocol version (VN: %u, vl: %d)", this->VN, frame->vec_limit);
			break;


		case 4:
			if ((this->VN == 4) && (this->DSTIP > 0) && (this->DSTIP <= 255)) // SOCK4A
			{
				// Parse HOST

				vec = ft_frame_get_vec_at(frame, 3);
				char * p = ft_vec_ptr(vec);	

				if (p[-1] == '\0')
				{
					sock_relay_on_socks4_request(this, established_sock, frame);
					return true;
				}

				// TODO: Ensure that capacity is sane
				// Keep filling this frame ...
				ft_frame_prev_vec(frame);
				vec->capacity += 1;
				vec->limit = vec->capacity;
				return false;
			}

			FT_ERROR("Unknown SOCKS protocol version (VN: %u, vl: %d)", this->VN, frame->vec_limit);
			break;

	}

	FT_WARN("Failure to parse SOCKS request, closing the connection");

	ft_frame_return(frame);
	ft_stream_cntl(established_sock, FT_STREAM_WRITE_SHUTDOWN);
	return true;
}


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

void on_error(struct ft_stream * established_sock)
{
	struct sock_relay * relay = (struct sock_relay *)established_sock->base.socket.data;
	//TODO: maybe even hard shutdown
	ft_stream_cntl(&relay->in, FT_STREAM_WRITE_SHUTDOWN);
	ft_stream_cntl(&relay->out, FT_STREAM_WRITE_SHUTDOWN);
}

struct ft_stream_delegate sock_relay_in_prologue_delegate =
{
	.get_read_frame = sock_relay_in_prologue_on_get_read_frame,
	.read = sock_relay_in_prologue_on_read,
	.error = on_error,
};

struct ft_stream_delegate sock_relay_in_delegate = 
{
	.read = on_read_in_to_out,
	.error = on_error,
};

struct ft_stream_delegate sock_relay_out_delegate = 
{
	.connected = sock_relay_on_sock_connected,
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

	FT_INFO("Stats OUT: Re:%u We:%u+%u Rb:%lu Wb:%lu",
		relay->out.stats.read_events,
		relay->out.stats.write_events,
		relay->out.stats.write_direct,
		relay->out.stats.read_bytes,
		relay->out.stats.write_bytes
	);

	ft_stream_fini(&relay->in);
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

	//ft_log_verbose(true);
	//ft_config.log_trace_mask |= FT_TRACE_ID_STREAM | FT_TRACE_ID_EVENT_LOOP;

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

