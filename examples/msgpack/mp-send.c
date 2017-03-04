#include <stdlib.h>
#include <stdbool.h>
#include <sys/un.h>

#include <ft.h>
#include <msgpack.h>

///

int msgpack_ft_vec_write(void * data, const char* buf, size_t len)
{
	struct ft_vec * vec = data;
	assert(data != NULL);

	void * dst = ft_vec_ptr(vec);
	assert(dst != NULL);

	if (!ft_vec_forward(vec, len)) return -1;

	memcpy(dst, buf, len);

	return 0;
}

///

bool on_read(struct ft_stream * established_sock, struct ft_frame * frame)
{
	if (frame->type == FT_FRAME_TYPE_RAW_DATA)
	{
		ft_frame_flip(frame);
		ft_frame_fwrite(frame, stdout);
	}

	ft_frame_return(frame);
	return true;
}

void on_error(struct ft_stream * established_sock)
{
	FT_FATAL("Error on the socket");
	exit(EXIT_FAILURE);
}

struct ft_stream_delegate stream_delegate = 
{
	.read = on_read,
	.error = on_error,
};

///

int main(int argc, char const *argv[])
{
	bool ok;
	struct ft_context context;
	struct ft_stream sock;

	ft_config.log_verbose = true;
	ft_initialise();
	
	//ft_config.log_trace_mask |= FT_TRACE_ID_STREAM | FT_TRACE_ID_EVENT_LOOP;


	// Initializa context
	ok = ft_context_init(&context);
	if (!ok) return EXIT_FAILURE;

	// Resolve target
	struct addrinfo target_addr;
	struct sockaddr_un un;
	un.sun_family = AF_UNIX;
	snprintf(un.sun_path, sizeof(un.sun_path)-1, "%s", "/tmp/scpb.tmp");

	target_addr.ai_family = AF_UNIX;
	target_addr.ai_socktype = SOCK_STREAM;
	target_addr.ai_protocol = 0;
	target_addr.ai_addr = (struct sockaddr *)&un;
	target_addr.ai_addrlen = sizeof(un);

	ok = ft_stream_connect(&sock, &stream_delegate, &context, &target_addr);
	if (!ok) return EXIT_FAILURE;

	struct ft_frame * frame = ft_pool_borrow(&context.frame_pool, FT_FRAME_TYPE_RAW_DATA);
	if (frame == NULL) return EXIT_FAILURE;

	ft_frame_format_simple(frame);
	struct ft_vec * vec = ft_frame_get_vec(frame);
	if (vec == NULL) return EXIT_FAILURE;

	msgpack_packer pk;
    msgpack_packer_init(&pk, vec, msgpack_ft_vec_write);

    msgpack_pack_array(&pk, 3);
    msgpack_pack_int(&pk, 1);
    msgpack_pack_true(&pk);
    msgpack_pack_str(&pk, 7);
    msgpack_pack_str_body(&pk, "example", 7);

	ft_frame_flip(frame);

	ok = ft_stream_write(&sock, frame);
	if (!ok) return EXIT_FAILURE;


	ok = ft_stream_cntl(&sock, FT_STREAM_WRITE_SHUTDOWN);
	if (!ok) return EXIT_FAILURE;

	// Enter event loop (will quit when all data are transmitted)
	ft_context_run(&context);

	// Clean up
	ft_context_fini(&context);

	return EXIT_SUCCESS;
}
