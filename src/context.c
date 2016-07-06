#include "all.h"

///

static void context_on_sighup(struct ev_loop * loop, ev_signal * w, int revents);
static void context_on_sigexit(struct ev_loop * loop, ev_signal * w, int revents);

///

bool context_init(struct context * this)
{
	bool ok;

	this->first_exiting_watcher = NULL;
	this->last_exiting_watcher = NULL;

	this->ev_loop = ev_default_loop(libsccmn_config.libev_loop_flags);
	if (this->ev_loop == NULL) return false;

	// Set a logging context
	logging_set_context(this);

	// Install signal handlers
	ev_signal_init(&this->sighup_w, context_on_sighup, SIGHUP);
	ev_signal_start(this->ev_loop, &this->sighup_w);
	ev_unref(this->ev_loop);
	this->sighup_w.data = this;

	ev_signal_init(&this->sigint_w, context_on_sigexit, SIGINT);
	ev_signal_start(this->ev_loop, &this->sigint_w);
	ev_unref(this->ev_loop);
	this->sigint_w.data = this;

	ev_signal_init(&this->sigterm_w, context_on_sigexit, SIGTERM);
	ev_signal_start(this->ev_loop, &this->sigterm_w);
	ev_unref(this->ev_loop);
	this->sigterm_w.data = this;

	heartbeat_init(&this->heartbeat, this);
	heartbeat_start(&this->heartbeat);

	ok = frame_pool_init(&this->frame_pool, &this->heartbeat);
	if (!ok) return false;

	return true;
}


void context_fini(struct context * this)
{
	logging_set_context(NULL);

	//TODO: Destroy heartbeat
	//TODO: Destroy frame pool
	//TODO: Uninstall signal handlers

	ev_loop_destroy(this->ev_loop);
	this->ev_loop = NULL;
}


static void context_on_sighup(struct ev_loop * loop, ev_signal * w, int revents)
{
	logging_reopen();
}

static void context_on_sigexit(struct ev_loop * loop, ev_signal * w, int revents)
{
	struct context * this = w->data;
	assert(this != NULL);

	if (w->signum == SIGINT) putchar('\n');
	
	for (struct exiting_watcher * watcher = this->first_exiting_watcher; watcher != NULL; watcher = watcher->next)
	{
		if (watcher->cb == NULL) continue;
		watcher->cb(watcher, this);
	}

	ev_ref(this->ev_loop);
	ev_signal_stop(this->ev_loop, &this->sigint_w);

	ev_ref(this->ev_loop);
	ev_signal_stop(this->ev_loop, &this->sigterm_w);
}


void context_evloop_run(struct context * this)
{
	assert(this != NULL);

	L_TRACE(L_TRACEID_EVENT_LOOP, "event loop start");

#if ((EV_VERSION_MINOR > 11) && (EV_VERSION_MAJOR >= 4))
	bool run=true;
	while (run)
	{
		run = ev_run(this->ev_loop, 0);
		if (run) L_TRACE(L_TRACEID_EVENT_LOOP, "event loop continue");
	}
#else
	ev_run(this->ev_loop, 0);
#endif

	L_TRACE(L_TRACEID_EVENT_LOOP, "event loop stop");
}

///

void context_exiting_watcher_add(struct context * this, struct exiting_watcher * watcher, exiting_cb cb)
{
	assert(this != NULL);
	assert(watcher != NULL);

	watcher->cb = cb;

	if (this->last_exiting_watcher == NULL)
	{
		assert(this->first_exiting_watcher == NULL);

		this->first_exiting_watcher = watcher;
		this->last_exiting_watcher = watcher;
		watcher->next = NULL;
		watcher->prev = NULL;
	}
	else
	{
		this->last_exiting_watcher->next = watcher;
		watcher->next = NULL;
		watcher->prev = this->last_exiting_watcher;
		this->last_exiting_watcher = watcher;
	}
}

void context_exiting_watcher_remove(struct context * this, struct exiting_watcher * watcher)
{
	assert(this != NULL);
	assert(watcher != NULL);

	if ((watcher->prev == NULL) && (watcher->next == NULL))
	{
		assert(this->first_exiting_watcher == watcher);
		assert(this->last_exiting_watcher == watcher);
		this->first_exiting_watcher = NULL;
		this->last_exiting_watcher = NULL;
		return;
	}

	assert((this->first_exiting_watcher != NULL) || (this->last_exiting_watcher != NULL));

	if (watcher->prev == NULL)
	{
		assert(this->first_exiting_watcher == watcher);
		this->first_exiting_watcher = watcher->next;
		if (this->first_exiting_watcher != NULL) this->first_exiting_watcher->prev = NULL;
	}
	else
	{
		watcher->prev->next = watcher->next;
	}

	if (watcher->next == NULL)
	{
		assert(this->last_exiting_watcher == watcher);
		this->last_exiting_watcher = watcher->prev;
		if (this->last_exiting_watcher != NULL) this->last_exiting_watcher->next = NULL;
	}
	else
	{
		watcher->next->prev = watcher->prev;
	}

	watcher->prev = NULL;
	watcher->next = NULL;
}
