#include "_ft_internal.h"

///

struct ft_context * ft_context_default = NULL;

///

// static void _ft_context_on_sigexit(struct ev_loop * loop, ev_signal * w, int revents);
// static void _ft_context_on_sighup(struct ev_loop * loop, ev_signal * w, int revents);
// static void ft_context_on_prepare(struct ev_loop * loop, ev_prepare * w, int revents);

static struct ft_timer_delegate _ft_context_shutdown_timer_delegate;
static struct ft_timer_delegate _ft_context_heartbeat_timer_delegate;

///

bool ft_context_init(struct ft_context * this)
{
	bool ok;
	assert(this != NULL);

	// this->ev_loop = ev_default_loop(ft_config.libev_loop_flags);
	// if (this->ev_loop == NULL) return false;

	this->flags.running = true;
	this->shutdown_counter = 0;

	this->active_watchers = NULL;
	this->now = ft_time();
	this->refs = 0;

	// Set a logging context
	ft_log_context(this);

	ok = ft_pubsub_init(&this->pubsub);

	// Install signal handlers
	// ev_signal_init(&this->sigint_w, _ft_context_on_sigexit, SIGINT);
	// ev_signal_start(this->ev_loop, &this->sigint_w);
	// ev_unref(this->ev_loop);
	// this->sigint_w.data = this;

	// ev_signal_init(&this->sigterm_w, _ft_context_on_sigexit, SIGTERM);
	// ev_signal_start(this->ev_loop, &this->sigterm_w);
	// ev_unref(this->ev_loop);
	// this->sigterm_w.data = this;

	// ev_signal_init(&this->sighup_w, _ft_context_on_sighup, SIGHUP);
	// ev_signal_start(this->ev_loop, &this->sighup_w);
	// ev_unref(this->ev_loop);
	// this->sighup_w.data = this;

	// Install heartbeat timer watcher
	ft_timer_init(&this->heartbeat_w, &_ft_context_heartbeat_timer_delegate, this, ft_config.heartbeat_interval, ft_config.heartbeat_interval);
	// ev_set_priority(&this->heartbeat_w, -2);
	ft_timer_start(&this->heartbeat_w);
	ft_unref(this);
	// this->heartbeat_w.data = this;
	this->heartbeat_at = 0.0;

	// Install proapre handler that allows to prepare for a event sleep
	// ev_prepare_init(&this->prepare_w, ft_context_on_prepare);
	// ev_prepare_start(this->ev_loop, &this->prepare_w);
	// ev_unref(this->ev_loop);
	// this->prepare_w.data = this;

	this->started_at = ft_now(this);
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

	if (ft_context_default == NULL) ft_context_default = this;

	ft_subscriber_subscribe(&this->frame_pool.heartbeat, &this->pubsub, FT_PUBSUB_TOPIC_HEARTBEAT);

	return true;
}


void ft_context_fini(struct ft_context * this)
{
	assert(this != NULL);
	
	this->now = ft_time();

	ft_subscriber_unsubscribe(&this->frame_pool.heartbeat);

	ft_log_finalise();

	ft_pool_fini(&this->frame_pool);
	ft_pubsub_fini(&this->pubsub);
	//TODO: Uninstall signal handlers

	if (ft_context_default == this) ft_context_default = NULL;

	// ev_loop_destroy(this->ev_loop);
	// this->ev_loop = NULL;
}


void ft_context_run(struct ft_context * this)
{
	assert(this != NULL);

	FT_TRACE(FT_TRACE_ID_EVENT_LOOP, "event loop start");


	for (;;) {

		// Publish 'prepare' message
		struct ft_pubsub_message_event_loop prepare_msg = {
			.context = this,
			.type = 'P', // For prepare
		};
		ft_pubsub_publish(&this->pubsub, FT_PUBSUB_TOPIC_EVENT_LOOP, &prepare_msg);

		// How many active watchers we have?
		int active_watchers_count = 0;
		for (struct ft_watcher * i = this->active_watchers; i != NULL; i = i->next) {
			active_watchers_count += 1;
		}

		// That's a limitation of  WaitForMultipleObjectsEx
		assert(active_watchers_count < MAXIMUM_WAIT_OBJECTS);

		// No active watchers, exit thet event loop
		if ((active_watchers_count + this->refs) <= 0)
			break;

		HANDLE handles[active_watchers_count];
		struct ft_watcher * watchers[active_watchers_count];

		int pos = 0;
		for (struct ft_watcher * w = this->active_watchers; w != NULL; w = w->next, pos += 1) {
			assert(pos < active_watchers_count);
			handles[pos] = w->handle;
			watchers[pos] = w;
		}

		DWORD ret = WaitForMultipleObjectsEx(
			active_watchers_count,
			handles,
			FALSE,
			INFINITE, // the time-out interval, in milliseconds
			FALSE // bAlertable
		);
		this->now = ft_time();

		// Publish 'check' message
		struct ft_pubsub_message_event_loop check_msg = {
			.context = this,
			.type = 'C', // For prepare
		};
		ft_pubsub_publish(&this->pubsub, FT_PUBSUB_TOPIC_EVENT_LOOP, &check_msg);

		if (ret < active_watchers_count) {
			struct ft_watcher * w = watchers[ret];
			w->callback(this, w);
		} else if (ret == -1) {
			FT_WARN_WINERROR_P("WaitForMultipleObjectsEx failed.");
			break;
		} else {
			FT_WARN("WaitForMultipleObjectsEx invalid/unknown return code:%ld", ret);
			break;
		}
		//TODO following 
		/*
		else if (ret == TIMEOUT_RETURN) {
			struct ft_pubsub_message_event_loop idle_msg = {
			.context = this,
			.type = 'I', // For idle
		};
		ft_pubsub_publish(&this->pubsub, FT_PUBSUB_TOPIC_EVENT_LOOP, &check_msg);

		}
		*/

		fflush(stdout);
		fflush(stderr);
	}

// #if ((EV_VERSION_MINOR > 11) && (EV_VERSION_MAJOR >= 4))
// 	bool run=true;
// 	while (run)
// 	{
// 		run = ev_run(this->ev_loop, 0);
// 		if (run) FT_TRACE(FT_TRACE_ID_EVENT_LOOP, "event loop continue");
// 	}
// #else
// 	ev_run(this->ev_loop, 0);
// #endif

	FT_TRACE(FT_TRACE_ID_EVENT_LOOP, "event loop stop");
}

///

void ft_context_stop(struct ft_context * this)
{
	FT_TRACE(FT_TRACE_ID_EVENT_LOOP, "BEGIN ft_context_stop");

	if (this->flags.running)
	{
		this->flags.running = false;
		this->shutdown_at = ft_now(this);

		struct ft_pubsub_message_exit msg = {
			.exit_phase = FT_EXIT_PHASE_POLITE
		};
		ft_pubsub_publish(&this->pubsub, FT_PUBSUB_TOPIC_EXIT, &msg);

		ft_timer_init(&this->shutdown_w, &_ft_context_shutdown_timer_delegate, this, 0.0, 2.0);
		ft_timer_start(&this->shutdown_w);
		ft_unref(this);
		// this->shutdown_w.data = this;

	}

	FT_TRACE(FT_TRACE_ID_EVENT_LOOP, "END ft_context_stop");
}

// static void _ft_context_on_sigexit(struct ev_loop * loop, ev_signal * w, int revents)
// {
// 	struct ft_context * this = w->data;
// 	assert(this != NULL);

// 	if (w->signum == SIGINT) putchar('\n');

// 	ft_context_stop(this);
// }


static void _ft_context_on_shutdown_timer(struct ft_timer * timer)
{
	assert(timer != NULL);

	struct ft_context * this = timer->watcher.context;
	assert(this != NULL);

	FT_TRACE(FT_TRACE_ID_EVENT_LOOP, "BEGIN _ft_context_on_shutdown_timer");

	this->shutdown_counter += 1;

	// Disable term. signal handlers - it allows the secondary signaling that has a bigger (unpolite) effect on the app
	// if (ev_is_active(&this->sigint_w))
	// {
	// 	ev_ref(this->ev_loop);
	// 	ev_signal_stop(this->ev_loop, &this->sigint_w);
	// }

	// if (ev_is_active(&this->sigterm_w))
	// {
	// 	ev_ref(this->ev_loop);
	// 	ev_signal_stop(this->ev_loop, &this->sigterm_w);
	// }

	// if (ev_is_active(&this->sighup_w))
	// {
	// 	ev_ref(this->ev_loop);
	// 	ev_signal_stop(this->ev_loop, &this->sighup_w);
	// }

	//TODO: Propagate this event -> it can be used for forceful shutdown

	FT_TRACE(FT_TRACE_ID_EVENT_LOOP, "END _ft_context_on_shutdown_timer");
}

static struct ft_timer_delegate _ft_context_shutdown_timer_delegate = 
{
	.tick = _ft_context_on_shutdown_timer,
	.fini = NULL,
};


///

static void _ft_context_on_heartbeat_timer(struct ft_timer * timer)
{
	assert(timer != NULL);

	struct ft_context * this = timer->watcher.context;
	assert(this != NULL);

	FT_TRACE(FT_TRACE_ID_EVENT_LOOP, "BEGIN _ft_context_on_heartbeat_timer");

	struct ft_pubsub_message_heartbeat msg = {
		.now = ft_now(this)
	};
	ft_pubsub_publish(&this->pubsub, FT_PUBSUB_TOPIC_HEARTBEAT, &msg);

	//Lag detector
	if (this->heartbeat_at > 0.0)
	{
		double delta = (msg.now - this->heartbeat_at) - timer->repeat;
		if (delta > ft_config.lag_detector_sensitivity)
		{
			FT_WARN("Lag (~ %.2lf sec.) detected", delta);
		}
	}
	this->heartbeat_at = msg.now;

	FT_TRACE(FT_TRACE_ID_EVENT_LOOP, "END _ft_context_on_heartbeat_timer");
}

static struct ft_timer_delegate _ft_context_heartbeat_timer_delegate = 
{
	.tick = _ft_context_on_heartbeat_timer,
	.fini = NULL,
};

// void ft_context_on_prepare(struct ev_loop * loop, ev_prepare * w, int revents)
// {
// 	struct ft_context * this = w->data;
// 	assert(this != NULL);

// 	FT_TRACE(FT_TRACE_ID_EVENT_LOOP, "BEGIN ft_context_on_prepare");

// 	ev_tstamp now = ev_now(loop);

// 	// Log on_prepare call
// 	if ((ft_config.log_backend != NULL) && (ft_config.log_backend->on_prepare != NULL))
// 	{
// 		ft_config.log_backend->on_prepare(this, now);
// 	}

// 	FT_TRACE(FT_TRACE_ID_EVENT_LOOP, "END ft_context_on_prepare");
// }


// void _ft_context_on_sighup(struct ev_loop * loop, ev_signal * w, int revents)
// {
// 	// Forward to logging
// 	if ((ft_config.log_backend != NULL) && (ft_config.log_backend->on_sighup != NULL))
// 	{
// 		ft_config.log_backend->on_sighup();
// 	}

// }


void ft_context_watcher_add(struct ft_context * this, struct ft_watcher * watcher) {
	assert(this != NULL);
	assert(watcher != NULL);
	assert(watcher->next == NULL);

	if (ft_context_watcher_is_added(this, watcher)) {
		FT_WARN("Watcher is alreadt added");
		return;
	}

	watcher->next = this->active_watchers;
	this->active_watchers = watcher;
}

void ft_context_watcher_remove(struct ft_context * this, struct ft_watcher * watcher) {
	assert(this != NULL);
	assert(watcher != NULL);
	assert(watcher->next != NULL);

	struct ft_watcher ** prev = &this->active_watchers;

	while (*prev != NULL) {
		if (*prev == watcher) {
			*prev = watcher->next;
			return;
		}

		prev = &(*prev)->next;
	}

	FT_WARN("Cannot find watcher in active watchers");
}

bool ft_context_watcher_is_added(struct ft_context * this, struct ft_watcher * watcher) {
	assert(this != NULL);
	assert(watcher != NULL);

	for (struct ft_watcher * i = this->active_watchers; i != NULL; i = i->next) {
		if (i == watcher) return true;
	}
	
	return false;
}
