#include <stdlib.h>
#include <stdbool.h>

#include <ft.h>

///

SSL_CTX * ssl_ctx;

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
	int rc;
	struct ft_context context;
	struct ft_stream sock;

	//ft_log_verbose(true);
	//ft_config.log_trace_mask |= FT_TRACE_ID_STREAM | FT_TRACE_ID_EVENT_LOOP;

	ft_initialise();	

	// Load nice OpenSSL error messages
	SSL_load_error_strings();
	ERR_load_crypto_strings();

	// Initialize context
	ok = ft_context_init(&context);
	if (!ok) return EXIT_FAILURE;

	// Initialize OpenSSL context
	ssl_ctx = SSL_CTX_new(SSLv23_client_method());
	if (ssl_ctx == NULL) return EXIT_FAILURE;

	rc = SSL_CTX_load_verify_locations(ssl_ctx, "curl-ca-bundle.crt", NULL);
	if (rc != 1) return EXIT_FAILURE;


	struct addrinfo * target_addr;
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


	ok = ft_stream_connect(&sock, &stream_delegate, &context, target_addr);
	if (!ok) return EXIT_FAILURE;

	freeaddrinfo(target_addr);

	ft_stream_set_partial(&sock, true);

	ok = ft_stream_enable_ssl(&sock, ssl_ctx);
	if (!ok) return EXIT_FAILURE;

	// Configure SNI
	SSL_set_tlsext_host_name(sock.ssl, argv[1]);

	struct ft_frame * frame = ft_pool_borrow(&context.frame_pool, FT_FRAME_TYPE_RAW_DATA);
	if (frame == NULL) return EXIT_FAILURE;

	ft_frame_format_simple(frame);
	struct ft_vec * vec = ft_frame_get_vec(frame);
	if (vec == NULL) return EXIT_FAILURE;

	ok = ft_vec_sprintf(vec, "GET / HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n", argv[1]);
	if (!ok) return EXIT_FAILURE;

	ft_frame_flip(frame);

	ok = ft_stream_write(&sock, frame);
	if (!ok) return EXIT_FAILURE;

	ok = ft_stream_cntl(&sock, FT_STREAM_WRITE_SHUTDOWN);
	if (!ok) return EXIT_FAILURE;

	// Enter event loop
	ft_context_run(&context);

	// Finalize context
	ft_context_fini(&context);

	return EXIT_SUCCESS;
}
