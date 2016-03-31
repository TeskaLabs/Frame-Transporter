#include "all.h"

static void heartbeat_on_timer(struct ev_loop * loop, ev_timer * w, int revents);

///

bool heartbeat_init(struct heartbeat * this, size_t size, ev_tstamp repeat)
{
	assert(this != NULL);

	this->cbs = NULL;
	this->size = size;
	this->top = 0;
	this->last_beat = 0.0;

	this->cbs = realloc(this->cbs, sizeof(heartbeat_cb) * this->size);
	if (this == NULL)
	{
		L_WARN_ERRNO(errno, "Failed to allocate memory for heartbeat object (size: %zu)", this->size);
		free(this);
		return false;
	}

	for (int i=0; i<this->size; i++)
	{
		(*this->cbs)[i] = NULL;
	}

	ev_timer_init(&this->timer_w, heartbeat_on_timer, 0.0, repeat);
	this->timer_w.data = this;

	return true;
}


bool heartbeat_fini(struct heartbeat * this)
{
	assert(this != NULL);
	if (this->cbs != NULL)
	{
		free(this->cbs);
		this->cbs = NULL;
	}
	return true;
}


bool heartbeat_start(struct ev_loop * loop, struct heartbeat * this)
{
	assert(loop != NULL);
	assert(this != NULL);

	ev_timer_start(loop, &this->timer_w);
	ev_unref(loop);
	return true;
}

bool heartbeat_stop(struct ev_loop * loop, struct heartbeat * this)
{
	assert(loop != NULL);
	assert(this != NULL);

	ev_ref(loop);
	ev_timer_stop(loop, &this->timer_w);
	return true;
}


bool heartbeat_add(struct heartbeat * this, heartbeat_cb cb)
{
	assert(this != NULL);

retry:
	for (int i=0; i<this->size; i++)
	{
		if ((*this->cbs)[i] != NULL) continue;
		(*this->cbs)[i] = cb;
		if ((i+1) > this->top) this->top = i+1;
		return true;
	}

	size_t old_size = this->size;
	this->size += 10;
	L_DEBUG("Extending heartbeat list size to %zu", this->size);

	this->cbs = realloc(this->cbs, sizeof(heartbeat_cb) * this->size);
	if (this == NULL)
	{
		L_WARN_ERRNO(errno, "Failed to allocate memory for heartbeat object (size: %zu)", this->size);
		free(this);
		return false;
	}

	for (int i=old_size; i<this->size; i++)
	{
		(*this->cbs)[i] = NULL;
	}
	goto retry;
}

bool heartbeat_remove(struct heartbeat * this, heartbeat_cb cb)
{
	assert(this != NULL);

	for (int i=0; i<this->size; i++)
	{
		if ((*this->cbs)[i] != cb) continue;
		(*this->cbs)[i] = NULL;
		return true;
	}

	return false;
}


void heartbeat_on_timer(struct ev_loop * loop, ev_timer * w, int revents)
{
	ev_tstamp now = ev_now(loop);
	struct heartbeat * this = w->data;

	for (int i=0; i<this->top; i++)
	{
		if ((*this->cbs)[i] == NULL) continue;
		(*this->cbs)[i](loop, now);
	}

	// Flush logs
	logging_flush();

	//Lag detector
	if (this->last_beat > 0.0)
	{
		double delta = (now - this->last_beat) - w->repeat;
		if (delta > libsccmn_config.lag_detector_sensitivity)
		{
			L_WARN("Lag (~ %.2lf sec.) detected", delta);
		}
	}
	this->last_beat = now;

}
