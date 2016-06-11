#include "all.h"

///

static void context_on_sighup(struct ev_loop * loop, ev_signal * w, int revents);
static void context_on_sigexit(struct ev_loop * loop, ev_signal * w, int revents);

///

bool context_init(struct context * this)
{
	bool ok;

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
	//TODO: Uninstall signel handlers

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
	
	ev_ref(this->ev_loop);
	ev_signal_stop(this->ev_loop, &this->sigint_w);

	ev_ref(this->ev_loop);
	ev_signal_stop(this->ev_loop, &this->sigterm_w);
}

void context_evloop_run(struct context * this)
{
	assert(this != NULL);

	bool run=true;
	while (run)
	{
		run = ev_run(this->ev_loop, 0);
	}
}
