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
	
	this->iocp_handle = CreateIoCompletionPort(
		INVALID_HANDLE_VALUE,  // create an I/O completion port without association
		NULL,                  // create a new I/O completion port
		0,                     // ignored in this case
		1                      // one concurent thread (a main thread)
	);
	if (this->iocp_handle == NULL) {
		FT_ERROR_WINERROR_P("CreateIoCompletionPort failed");
		return false;
	}


	this->flags.running = true;
	this->shutdown_counter = 0;

	this->active_timers = NULL;
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

	// Stop all timers
	while (this->active_timers != NULL) {
		ft_timer_stop(this->active_timers);
	}

	if (ft_context_default == this) ft_context_default = NULL;

	if (this->iocp_handle != NULL) {
		BOOL ok = CloseHandle(this->iocp_handle);
		if (!ok) FT_WARN_WINERROR_P("CloseHandle(iocp_handle) failed");
		this->iocp_handle = NULL;
	}

	// ev_loop_destroy(this->ev_loop);
	// this->ev_loop = NULL;
}


static void ft_context_event_loop_publish(struct ft_context * this, char type) {
	struct ft_pubsub_message_event_loop message = {
		.context = this,
		.type = type,
	};
	ft_pubsub_publish(&this->pubsub, FT_PUBSUB_TOPIC_EVENT_LOOP, &message);
}


void ft_context_run(struct ft_context * this)
{
	assert(this != NULL);

	BOOL ok;

	FT_TRACE(FT_TRACE_ID_EVENT_LOOP, "event loop start");

	for (;;) {
		fflush(stdout);
		fflush(stderr);

		this->now = ft_time();
		double delta_time = ft_timers_process(this, NULL);
		if (delta_time > 3.0) { // That's an idle time
			delta_time = 3.0;
		}

		// Publish 'prepare' message
		ft_context_event_loop_publish(this, 'P'); // For prepare

		DWORD n_bytes = -1;
		ULONG_PTR completion_key = 0;
		OVERLAPPED * overlapped = NULL;

		ok = GetQueuedCompletionStatus(
			this->iocp_handle,
			&n_bytes,
			(PULONG_PTR)&completion_key,
			&overlapped,
			delta_time * 1000 // miliseconds
		);

		if (!ok) {
			DWORD error = GetLastError();
			
			ft_context_event_loop_publish(this, 'C'); // For check

			switch (error) {

				case WAIT_TIMEOUT:
					this->now = ft_time();

					unsigned int trigger_counter = 0;
					ft_timers_process(this, &trigger_counter);
					if (trigger_counter == 0) {
						// We waited for event and nothing come, so let's signalize the idle state
						ft_context_event_loop_publish(this, 'I'); // For idle
					}

					continue;


				default:
					if (overlapped != NULL) {
						struct ft_iocp_watcher * watcher = (struct ft_iocp_watcher *)overlapped;
						assert(watcher != NULL);
						watcher->delegate->error(this, watcher, error, n_bytes, completion_key);
					} else {
						FT_WARN_WINERROR_P("GetQueuedCompletionStatus failed");
					}
					break;
			}
		}

		ft_context_event_loop_publish(this, 'C'); // For check

		// This is possible because overlapped is the first field in the struct ft_iocp_watcher;
		struct ft_iocp_watcher * watcher = (struct ft_iocp_watcher *)overlapped;
		assert(watcher != NULL);
		watcher->delegate->completed(this, watcher, n_bytes, completion_key);

	}

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

	struct ft_context * this = timer->context;
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

	struct ft_context * this = timer->context;
	assert(this != NULL);

	FT_TRACE(FT_TRACE_ID_EVENT_LOOP, "BEGIN _ft_context_on_heartbeat_timer");

	this->heartbeat_at = ft_now(this);
	struct ft_pubsub_message_heartbeat message = {
		.context = this,
	};
	ft_pubsub_publish(&this->pubsub, FT_PUBSUB_TOPIC_HEARTBEAT, &message);


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

