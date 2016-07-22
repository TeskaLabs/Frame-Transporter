#include "all.h"

///

struct application app = {
	.config = {
		.initialized = false,
		.config_file = "./httpserver.conf",
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

///

int main(int argc, char const *argv[])
{
	bool ok;

	//ft_log_verbose(true);
	//ft_config.log_trace_mask |= FT_TRACE_ID_STREAM | FT_TRACE_ID_EVENT_LOOP;

	ft_initialise();
	
	// Initializa context
	ok = ft_context_init(&app.context);
	if (!ok) return EXIT_FAILURE;

	// Initialize a list for listening sockets
	ok = listen_init(&app.listen, &app.context);
	if (!ok) return EXIT_FAILURE;

	ok = ft_list_init(&app.connections, connection_fini_list);
	if (!ok) return EXIT_FAILURE;

	ok = application_configure(&app);
	if (!ok) return EXIT_FAILURE;

#ifdef MAP_HUGETLB
	FT_INFO("Using hugetlb pages!");
	ft_pool_set_alloc(&app.context.frame_pool, ft_pool_alloc_hugetlb);
#endif

	// Install termination handler
	ft_context_at_termination(&app.context, on_termination_cb, &app);

	// Registed check handler
//	ev_prepare prepare_w;
//	ev_prepare_init(&prepare_w, on_check_cb);
//	ev_prepare_start(context.ev_loop, &prepare_w);
//	ev_unref(context.ev_loop);

	ok = listen_start(&app.listen);
	if (!ok) return EXIT_FAILURE;

	// Enter event loop
	ft_context_run(&app.context);

	// Clean up
	listen_fini(&app.listen);
	ft_context_fini(&app.context);

	return EXIT_SUCCESS;
}
