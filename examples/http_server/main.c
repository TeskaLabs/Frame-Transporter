#include "all.h"

///

struct application app = {
	.ssl_ctx = NULL,

	.config = {
		.initialized = false,
		.config_file = "./httpserver.conf",

		.ssl_used = false,
		.ssl_key_file = "./key.pem",
		.ssl_cert_file = "./cert.pem",
	}
};

///

static void on_termination_cb(struct ft_context * context, void * data)
{
	struct application * app = data;
	assert(app != NULL);

	FT_LIST_FOR(&app->connections, node)
	{
		struct connection * connection = (struct connection *)node->data;
		connection_terminate(connection);
	}
}

static void on_check_cb(struct ev_loop * loop, ev_prepare * check, int revents)
{
	// Called periodically during each event loop

	struct application * app = (struct application *)check->data;
	assert(app != NULL);

restart:
	FT_LIST_FOR(&app->connections, node)
	{
		struct connection * connection = (struct connection *)node->data;
		if (connection_is_closed(connection))
		{
			ft_list_remove(&app->connections, node);
			goto restart; // List has been changed during iteration
		}
	}
}

///

int main(int argc, char const *argv[])
{
	bool ok;
	int rc;

	//ft_log_verbose(true);
	//ft_config.log_trace_mask |= FT_TRACE_ID_MEMPOOL;

	ft_initialise();
	
	// Initializa context
	ok = ft_context_init(&app.context);
	if (!ok) return EXIT_FAILURE;

	ft_pidfile_filename("./httpserver.pid");
	pid_t isrun = ft_pidfile_is_running();
	if (isrun >= 0)
	{
		FT_FATAL("Pid file exists, server is already running (process pid: %d)?", isrun);
		return EXIT_FAILURE;
	}

	// Initialize a list for listening sockets
	ok = listen_init(&app.listen, &app.context);
	if (!ok) return EXIT_FAILURE;

	ok = ft_list_init(&app.connections, connection_fini_list);
	if (!ok) return EXIT_FAILURE;

	ok = application_configure(&app);
	if (!ok) return EXIT_FAILURE;

	// Initialize OpenSSL context if needed
	if (app.config.ssl_used)
	{
		app.ssl_ctx = SSL_CTX_new(SSLv23_server_method());
		if (app.ssl_ctx == NULL) return EXIT_FAILURE;

		long ssloptions = SSL_OP_ALL | SSL_OP_NO_SESSION_RESUMPTION_ON_RENEGOTIATION | SSL_OP_NO_COMPRESSION
		| SSL_OP_CIPHER_SERVER_PREFERENCE | SSL_OP_SINGLE_DH_USE;
		SSL_CTX_set_options(app.ssl_ctx, ssloptions);

		rc = SSL_CTX_use_PrivateKey_file(app.ssl_ctx, app.config.ssl_key_file, SSL_FILETYPE_PEM);
		if (rc != 1)
		{
			FT_FATAL_OPENSSL("Loading private key from '%s'", app.config.ssl_key_file);
			return EXIT_FAILURE;
		}

		rc = SSL_CTX_use_certificate_file(app.ssl_ctx, app.config.ssl_cert_file, SSL_FILETYPE_PEM);
		if (rc != 1)
		{
			FT_FATAL_OPENSSL("Loading certificate from '%s'", app.config.ssl_cert_file);
			return EXIT_FAILURE;
		}
	}

#ifdef MAP_HUGETLB
	FT_INFO("Using hugetlb pages!");
	ft_pool_set_alloc(&app.context.frame_pool, ft_pool_alloc_hugetlb);
#endif
	// Install termination handler
	ft_context_at_termination(&app.context, on_termination_cb, &app);

	// Registed check handler
	ev_prepare prepare_w;
	ev_prepare_init(&prepare_w, on_check_cb);
	ev_prepare_start(app.context.ev_loop, &prepare_w);
	prepare_w.data = &app;
	ev_unref(app.context.ev_loop);

	ok = listen_start(&app.listen);
	if (!ok) return EXIT_FAILURE;

	// Daemonise
	pid_t pid = ft_daemonise(&app.context);
	if (pid == -1) return EXIT_FAILURE;
	if (pid > 0) return EXIT_SUCCESS; // Exit parent process

	ok = ft_pidfile_create();
	if (!ok) return EXIT_FAILURE;

	// Enter event loop
	ft_context_run(&app.context);

	// Clean up
	listen_fini(&app.listen);
	ft_context_fini(&app.context);

	ok = ft_pidfile_remove();
	if (!ok) return EXIT_FAILURE;

	return EXIT_SUCCESS;
}
