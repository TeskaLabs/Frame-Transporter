#include "_ft_internal.h"

///

static void _ft_context_on_sigexit(struct ev_loop * loop, ev_signal * w, int revents);
static void _ft_context_on_sighup(struct ev_loop * loop, ev_signal * w, int revents);
static void _ft_context_on_heartbeat_timer(struct ev_loop * loop, ev_timer * w, int revents);
static void _ft_context_on_shutdown_timer(struct ev_loop * loop, ev_timer * w, int revents);
static void ft_context_on_prepare(struct ev_loop * loop, ev_prepare * w, int revents);

///

bool ft_context_init(struct ft_context * this)
{
	bool ok;
	assert(this != NULL);

	this->ev_loop = ev_default_loop(ft_config.libev_loop_flags);
	if (this->ev_loop == NULL) return false;

	this->flags.running = true;
	this->shutdown_counter = 0;
	this->on_exit_list = NULL;

	// Set a logging context
	ft_log_context(this);

	// Install signal handlers
	ev_signal_init(&this->sigint_w, _ft_context_on_sigexit, SIGINT);
	ev_signal_start(this->ev_loop, &this->sigint_w);
	ev_unref(this->ev_loop);
	this->sigint_w.data = this;

	ev_signal_init(&this->sigterm_w, _ft_context_on_sigexit, SIGTERM);
	ev_signal_start(this->ev_loop, &this->sigterm_w);
	ev_unref(this->ev_loop);
	this->sigterm_w.data = this;

	ev_signal_init(&this->sighup_w, _ft_context_on_sighup, SIGHUP);
	ev_signal_start(this->ev_loop, &this->sighup_w);
	ev_unref(this->ev_loop);
	this->sighup_w.data = this;

	// Install heartbeat timer watcher
	ev_timer_init(&this->heartbeat_w, _ft_context_on_heartbeat_timer, 0.0, ft_config.heartbeat_interval);
	ev_set_priority(&this->heartbeat_w, -2);
	ev_timer_start(this->ev_loop, &this->heartbeat_w);
	ev_unref(this->ev_loop);
	this->heartbeat_w.data = this;
	this->heartbeat_at = 0.0;

	// Install proapre handler that allows to prepare for a event sleep
	ev_prepare_init(&this->prepare_w, ft_context_on_prepare);
	ev_prepare_start(this->ev_loop, &this->prepare_w);
	ev_unref(this->ev_loop);
	this->prepare_w.data = this;

	this->started_at = ev_now(this->ev_loop);
	this->shutdown_at = NAN;

	ok = ft_pool_init(&this->frame_pool);
	if (!ok) return false;

	if (ft_config.stream_ssl_ex_data_index == -2)
	{
		ft_config.stream_ssl_ex_data_index = SSL_get_ex_new_index(0, "stream_ssl_ex_data", NULL, NULL, NULL);	
		if (ft_config.stream_ssl_ex_data_index == -1)
		{
			ft_config.stream_ssl_ex_data_index = -2;
			FT_ERROR_OPENSSL("SSL_get_ex_new_index");
			return false;
		}
	}

	return true;
}


void ft_context_fini(struct ft_context * this)
{
	ft_log_finalise();

	//TODO: Destroy frame pool
	//TODO: Uninstall signal handlers

	ev_loop_destroy(this->ev_loop);
	this->ev_loop = NULL;
}


void ft_context_run(struct ft_context * this)
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

///

// This is a polite way of termination
// It doesn't guarantee that event loop is stopped, there can be a rogue watcher still running
static void _ft_context_terminate(struct ft_context * this, struct ev_loop * loop)
{
	FT_TRACE(FT_TRACE_ID_EVENT_LOOP, "BEGIN _ft_context_terminate");

	if (this->flags.running)
	{
		this->flags.running = false;
		this->shutdown_at = ev_now(loop);

		ft_exit_invoke_all(this->on_exit_list, this, FT_EXIT_PHASE_POLITE);

		ev_timer_init(&this->shutdown_w, _ft_context_on_shutdown_timer, 0.0, 2.0);
		ev_timer_start(this->ev_loop, &this->shutdown_w);
		ev_unref(this->ev_loop);
		this->shutdown_w.data = this;

	}

	FT_TRACE(FT_TRACE_ID_EVENT_LOOP, "END _ft_context_terminate");
}

static void _ft_context_on_sigexit(struct ev_loop * loop, ev_signal * w, int revents)
{
	struct ft_context * this = w->data;
	assert(this != NULL);

	if (w->signum == SIGINT) putchar('\n');

	_ft_context_terminate(this, loop);
}

void _ft_context_on_shutdown_timer(struct ev_loop * loop, ev_timer * w, int revents)
{
	struct ft_context * this = w->data;
	assert(this != NULL);

	FT_TRACE(FT_TRACE_ID_EVENT_LOOP, "BEGIN _ft_context_on_shutdown_timer");

	this->shutdown_counter += 1;

	// Disable term. signal handlers - it allows the secondary signaling that has a bigger (unpolite) effect on the app
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

	if (ev_is_active(&this->sighup_w))
	{
		ev_ref(this->ev_loop);
		ev_signal_stop(this->ev_loop, &this->sighup_w);
	}

	//TODO: Propagate this event -> it can be used for forceful shutdown

	FT_TRACE(FT_TRACE_ID_EVENT_LOOP, "END _ft_context_on_shutdown_timer");
}

///

void _ft_context_on_heartbeat_timer(struct ev_loop * loop, ev_timer * w, int revents)
{
	struct ft_context * this = w->data;
	assert(this != NULL);

	FT_TRACE(FT_TRACE_ID_EVENT_LOOP, "BEGIN _ft_context_on_heartbeat_timer");

	ev_tstamp now = ev_now(loop);

	ft_pool_heartbeat(&this->frame_pool, now);

	//Lag detector
	if (this->heartbeat_at > 0.0)
	{
		double delta = (now - this->heartbeat_at) - w->repeat;
		if (delta > ft_config.lag_detector_sensitivity)
		{
			FT_WARN("Lag (~ %.2lf sec.) detected", delta);
		}
	}
	this->heartbeat_at = now;

	FT_TRACE(FT_TRACE_ID_EVENT_LOOP, "END _ft_context_on_heartbeat_timer");
}

void ft_context_on_prepare(struct ev_loop * loop, ev_prepare * w, int revents)
{
	struct ft_context * this = w->data;
	assert(this != NULL);

	FT_TRACE(FT_TRACE_ID_EVENT_LOOP, "BEGIN ft_context_on_prepare");

	ev_tstamp now = ev_now(loop);

	// Log on_prepare call
	if ((ft_config.log_backend != NULL) && (ft_config.log_backend->on_prepare != NULL))
	{
		ft_config.log_backend->on_prepare(this, now);
	}

	FT_TRACE(FT_TRACE_ID_EVENT_LOOP, "END ft_context_on_prepare");
}

void _ft_context_on_sighup(struct ev_loop * loop, ev_signal * w, int revents)
{
	// Forward to logging
	if ((ft_config.log_backend != NULL) && (ft_config.log_backend->on_sighup != NULL))
	{
		ft_config.log_backend->on_sighup();
	}

}

ev_tstamp ft_safe_now(struct ft_context * this)
{
	return ((this != NULL) && (this->ev_loop != NULL)) ? ev_now(this->ev_loop) : ev_time();
}
