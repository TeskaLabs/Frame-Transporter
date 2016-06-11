#include "all.h"

///

bool context_init(struct context * this)
{
	bool ok;

	this->ev_loop = ev_default_loop(libsccmn_config.libev_loop_flags);
	if (this->ev_loop == NULL) return false;

	// Set a logging context
	logging_set_context(this);

	heartbeat_init(&this->heartbeat);

	ok = frame_pool_init(&this->frame_pool, &this->heartbeat, NULL);
	if (!ok) return false;

	return true;
}


void context_fini(struct context * this)
{
	logging_set_context(NULL);

	//TODO: Destroy heartbeat
	//TODO: Destroy frame pool

	ev_loop_destroy(this->ev_loop);
	this->ev_loop = NULL;
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
