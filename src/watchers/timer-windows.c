#include "../_ft_internal.h"

static void ft_timer_watcher_cb(struct ft_context * context, struct ft_watcher * watcher);

bool ft_timer_init(struct ft_timer * this, const struct ft_timer_delegate * delegate, struct ft_context * context, ev_tstamp after, ev_tstamp repeat) {
	assert(this != NULL); 
	assert(delegate != NULL); 
	assert(context != NULL); 

	this->watcher.next = NULL;
	this->watcher.data = NULL;
	this->watcher.callback = ft_timer_watcher_cb;
	this->watcher.context = context;

	this->watcher.handle = CreateWaitableTimer(NULL, TRUE, NULL);
	if (this->watcher.handle == NULL)
	{
		FT_ERROR_WINERROR_P("CreateWaitableTimer() failed");
		return false;
	}

	this->delegate = delegate;
	this->repeat = repeat;
	this->after = after;

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

	if (this->watcher.handle != NULL) {
		BOOL ok = CloseHandle(this->watcher.handle);
		this->watcher.handle = NULL;
		if (ok != TRUE) FT_WARN_WINERROR_P("CloseHandle() failed");
	}

}


static bool ft_timer_set(struct ft_timer * this) {
	// 100 nanosecond intervals 
	LARGE_INTEGER liDueTime;
	liDueTime.QuadPart = this->after * -1e7;

	BOOL ok = SetWaitableTimer(
		this->watcher.handle,
		&liDueTime,
		0,
		NULL,
		NULL,
		0
	);
	if (!ok) {
		FT_WARN_WINERROR_P("SetWaitableTimer() failed");
		return false;
	}

	return true;
}

bool ft_timer_start(struct ft_timer * this) {
	assert(this != NULL);

	bool ok = ft_timer_set(this);
	if (!ok) return false;

	ft_context_watcher_add(this->watcher.context, &this->watcher);

	return true;
}	


bool ft_timer_stop(struct ft_timer * this) {
	assert(this != NULL);

	BOOL ok = CancelWaitableTimer(this->watcher.handle);
	if (!ok) {
		FT_WARN_WINERROR_P("CancelWaitableTimer() failed");
		return false;
	}

	ft_context_watcher_remove(this->watcher.context, &this->watcher);

	return true;
}

bool ft_timer_is_started(struct ft_timer * this) {
	assert(this != NULL);
	return ft_context_watcher_is_added(this->watcher.context, &this->watcher);
}


void ft_timer_watcher_cb(struct ft_context * context, struct ft_watcher * watcher) {
	struct ft_timer * this = (struct ft_timer *) watcher;
	this->repeat = this->after;

	BOOL ok = CancelWaitableTimer(this->watcher.handle);
	if (!ok) {
		FT_WARN_WINERROR_P("CancelWaitableTimer() failed");
	}

	this->delegate->tick(this);

	ft_timer_set(this);
}
