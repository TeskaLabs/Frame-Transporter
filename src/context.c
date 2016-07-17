#include "_ft_internal.h"

///

static void _ft_context_on_sighup(struct ev_loop * loop, ev_signal * w, int revents);
static void _ft_context_on_sigexit(struct ev_loop * loop, ev_signal * w, int revents);

struct _ft_context_on_termination_entry
{
	ft_context_on_termination_callback callback;
	void * data;
};

///

bool context_init(struct context * this)
{
	bool ok;

	ok = ft_list_init(&this->on_termination_list, NULL);
	if (!ok) return false;

	this->ev_loop = ev_default_loop(ft_config.libev_loop_flags);
	if (this->ev_loop == NULL) return false;

	// Set a logging context
	ft_log_context(this);

	// Install signal handlers
	ev_signal_init(&this->sighup_w, _ft_context_on_sighup, SIGHUP);
	ev_signal_start(this->ev_loop, &this->sighup_w);
	ev_unref(this->ev_loop);
	this->sighup_w.data = this;

	ev_signal_init(&this->sigint_w, _ft_context_on_sigexit, SIGINT);
	ev_signal_start(this->ev_loop, &this->sigint_w);
	ev_unref(this->ev_loop);
	this->sigint_w.data = this;

	ev_signal_init(&this->sigterm_w, _ft_context_on_sigexit, SIGTERM);
	ev_signal_start(this->ev_loop, &this->sigterm_w);
	ev_unref(this->ev_loop);
	this->sigterm_w.data = this;

	heartbeat_init(&this->heartbeat, this);
	heartbeat_start(&this->heartbeat);

	ok = frame_pool_init(&this->frame_pool, &this->heartbeat);
	if (!ok) return false;

	if (ft_config.sock_est_ssl_ex_data_index == -2)
	{
		ft_config.sock_est_ssl_ex_data_index = SSL_get_ex_new_index(0, "ft_stream_ptr", NULL, NULL, NULL);	
		if (ft_config.sock_est_ssl_ex_data_index == -1)
		{
			ft_config.sock_est_ssl_ex_data_index = -2;
			FT_ERROR_OPENSSL("SSL_get_ex_new_index");
			return false;
		}
	}

	return true;
}


void context_fini(struct context * this)
{
	ft_log_context(NULL);

	//TODO: Destroy heartbeat
	//TODO: Destroy frame pool
	//TODO: Uninstall signal handlers
	//TODO: remove entries in on_termination_list

	ev_loop_destroy(this->ev_loop);
	this->ev_loop = NULL;
}


// This is a polite way of termination
// It doesn't guarantee that event loop is stopped, there can be a rogue watcher still running
static void _ft_context_terminate(struct context * this, struct ev_loop * loop)
{
	if (ev_is_active(&this->sigint_w))
	{
		ev_ref(this->ev_loop);
		ev_signal_stop(this->ev_loop, &this->sigint_w);
	}

	if (ev_is_active(&this->sigterm_w))
	{
		ev_ref(this->ev_loop);
		ev_signal_stop(this->ev_loop, &this->sigterm_w);
	}

	FT_LIST_FOR(&this->on_termination_list, node)
	{
		struct _ft_context_on_termination_entry * e = (struct _ft_context_on_termination_entry *)node->data;
		e->callback(this, e->data);
	}
}


static void _ft_context_on_sighup(struct ev_loop * loop, ev_signal * w, int revents)
{
	ft_log_reopen();
}


static void _ft_context_on_sigexit(struct ev_loop * loop, ev_signal * w, int revents)
{
	struct context * this = w->data;
	assert(this != NULL);

	if (w->signum == SIGINT) putchar('\n');

	_ft_context_terminate(this, loop);
}


void context_evloop_run(struct context * this)
{
	assert(this != NULL);

	FT_TRACE(FT_TRACE_ID_EVENT_LOOP, "event loop start");

#if ((EV_VERSION_MINOR > 11) && (EV_VERSION_MAJOR >= 4))
	bool run=true;
	while (run)
	{
		run = ev_run(this->ev_loop, 0);
		if (run) FT_TRACE(FT_TRACE_ID_EVENT_LOOP, "event loop continue");
	}
#else
	ev_run(this->ev_loop, 0);
#endif

	FT_TRACE(FT_TRACE_ID_EVENT_LOOP, "event loop stop");
}


bool ft_context_at_termination(struct context * this, ft_context_on_termination_callback callback, void * data)
{
	assert(this != NULL);
	assert(callback != NULL);

	struct ft_list_node * node = ft_list_node_new(sizeof(struct _ft_context_on_termination_entry));
	if (node == NULL) return false;
	struct _ft_context_on_termination_entry * e = (struct _ft_context_on_termination_entry *)node->data;

	e->callback = callback;
	e->data = data;

	ft_list_add(&this->on_termination_list, node);	

	return true;
}

