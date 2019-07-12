#include "_ft_internal.h"

bool ft_timer_init(struct ft_timer * this, const struct ft_timer_delegate * delegate, struct ft_context * context, double after, double repeat) {
	assert(this != NULL); 
	assert(delegate != NULL); 
	assert(context != NULL); 

	this->next = NULL;
	this->data = NULL;
	this->context = context;
	this->delegate = delegate;
	this->after = ft_now(context) + after;
	this->repeat = repeat;

	return true;
}


void ft_timer_fini(struct ft_timer * this) {
	assert(this != NULL);

	if (ft_timer_is_started(this)) {
		ft_timer_stop(this);
	}

	if (this->delegate->fini != NULL) {
		this->delegate->fini(this);
	}

}


void ft_timer_again(struct ft_timer * this) {
	if (this->repeat <= 0.0) {
		this->after = INFINITY;
		return;
	}

	double now = ft_now(this->context);
	while (this->after <= now) {
		this->after += this->repeat;
	}
}


bool ft_timer_start(struct ft_timer * this) {
	assert(this != NULL);

	if (ft_timer_is_started(this)) {
		FT_WARN("Timer is already started");
		return true;
	}

	ft_timer_again(this);

	this->next = this->context->active_timers;
	this->context->active_timers = this;

	return true;
}	


bool ft_timer_stop(struct ft_timer * this) {
	assert(this != NULL);

	struct ft_timer ** prev = &this->context->active_timers;

	while (*prev != NULL) {
		if (*prev == this) {
			*prev = this->next;
			return true;
		}

		prev = &(*prev)->next;
	}

	FT_WARN("Cannot find timer among started timers");

	return false;
}


bool ft_timer_is_started(struct ft_timer * this) {
	assert(this != NULL);

	for (struct ft_timer * t = this->context->active_timers; t != NULL; t = t->next) {
		if (t == this) {
			return true;
		}
	}

	return false;
}


static void ft_timer_trigger(struct ft_timer * this) {
	assert(this != NULL);
	ft_timer_again(this);

	this->delegate->tick(this);
}


double ft_timers_process(struct ft_context * this, unsigned int * trigger_counter) {
	double now = ft_now(this);
	double best_delta;

restart:
	best_delta = INFINITY;
	for (struct ft_timer * timer = this->active_timers; timer != NULL; timer = timer->next) {
		if (timer->after <= now) {
			if (trigger_counter != NULL) *trigger_counter += 1;
			ft_timer_trigger(timer);
			goto restart;
		}

		double delta = timer->after - now;
		if (delta < best_delta) best_delta = delta;
	}

	return best_delta;
}