#include "all.h"

///

static int connection_on_message_begin(http_parser * parser)
{
	struct connection * this = (struct connection *)parser->data;
	assert(this != NULL);

	assert(this->response_frame == NULL);

	this->response_frame = ft_pool_borrow(&app.context.frame_pool, FT_FRAME_TYPE_RAW_DATA);
	if (this->response_frame == NULL) return -1;

	ft_frame_format_simple(this->response_frame);
	struct ft_vec * vec = ft_frame_get_vec(this->response_frame);
	if (vec == NULL) return -1;

	ft_vec_sprintf(vec, "HTTP/1.1 200 OK\r\nConnection: close\r\nContent-type: text/plain\r\n\r\nHello World\r\n");

	return 0;
}

static int connection_on_message_complete(http_parser * parser)
{
	struct connection * this = (struct connection *)parser->data;
	assert(this != NULL);
	assert(this->response_frame != NULL);

	bool ok;

	ft_frame_flip(this->response_frame);

	ok = ft_stream_write(&this->stream, this->response_frame);
	if (!ok) return -1;
	this->response_frame = NULL;

	ok = ft_stream_cntl(&this->stream, FT_STREAM_WRITE_SHUTDOWN);
	if (!ok) return -1;

	return 0;
}

static http_parser_settings http_request_parser_settings =
{
	.on_message_begin = connection_on_message_begin,
	.on_message_complete = connection_on_message_complete,
};

///

bool connection_on_read(struct ft_stream * stream, struct ft_frame * frame)
{
	struct connection * this = (struct connection *)stream->data;
	assert(this != NULL);
	
	int nparsed;
	ft_frame_flip(frame);
	struct ft_vec * vec = ft_frame_get_vec(frame);
	size_t recved = (vec != NULL) ? ft_vec_len(vec) : 0;

	switch (frame->type)
	{
		case FT_FRAME_TYPE_RAW_DATA:
			assert(frame->vec_limit == 1);
			// Start up / continue the parser
			nparsed = http_parser_execute(&this->http_request_parser, &http_request_parser_settings, ft_vec_ptr(vec), recved);
			break;

		case FT_FRAME_TYPE_STREAM_END:
			// Signalise that EOF has been received
			nparsed = http_parser_execute(&this->http_request_parser, &http_request_parser_settings, NULL, 0);
			break;

		default:
			FT_ERROR("Unknown/invalid frame type: %d", frame->type);
			ft_frame_return(frame);
			return true;
	}


	if (this->http_request_parser.upgrade)
	{
		// handle new protocol
		FT_WARN("Upgrade to a new protocol is not supported");
		//TODO: Close connection
	}

	else if (nparsed != recved)
	{
		FT_WARN("Parser error");
		//TODO: Close connection
	}


	ft_frame_return(frame);
	return true;
}

static struct ft_stream_delegate stream_delegate =
{
	.get_read_frame = NULL,
	.read = connection_on_read,

	.connected = NULL,
	.fini = NULL,
	.error = NULL,
};

///

bool connection_is_closed(struct connection * this)
{
	return (ft_stream_is_shutdown(&this->stream));
}

///

bool connection_init(struct connection * this, struct ft_listener * listening_socket, int fd, const struct sockaddr * peer_addr, socklen_t peer_addr_len)
{
	bool ok;
	
	assert(this != NULL);
	assert(listening_socket != NULL);
	assert(fd >= 0);

	this->response_frame = NULL;

	ok = ft_stream_accept(&this->stream, &stream_delegate, listening_socket, fd, peer_addr, peer_addr_len);
	if (!ok) return false;
	this->stream.data = this;

	// Start read on the socket
	ft_stream_set_partial(&this->stream, true);

	// Prepare parser
	http_parser_init(&this->http_request_parser, HTTP_REQUEST);
	this->http_request_parser.data = this;

	return true;
}

static void connection_fini(struct connection * this)
{
	assert(this != NULL);

	if (this->response_frame != NULL)
	{
		ft_frame_return(this->response_frame);
		this->response_frame = NULL; 
	}

	ft_stream_fini(&this->stream);
}


void connection_fini_list(struct ft_list * list, struct ft_list_node * node)
{
	struct connection * stream = (struct connection *)node->data;
	assert(stream != NULL);

	connection_fini(stream);	
}

void connection_terminate(struct connection * this)
{
	assert(this != NULL);

	ft_stream_cntl(&this->stream, FT_STREAM_WRITE_SHUTDOWN);
}
