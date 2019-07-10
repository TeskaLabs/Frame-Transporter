#ifndef FT_WATCHER_TIMER_H_
#define FT_WATCHER_TIMER_H_

struct ft_timer;

struct ft_timer_delegate
{
	void (*tick)(struct ft_timer *); // Mandatory
	void (*fini)(struct ft_timer *); // Optional
};


struct ft_timer {
	struct ft_watcher watcher;
	const struct ft_timer_delegate * delegate;

	ev_tstamp after;
	ev_tstamp repeat;
};

bool ft_timer_init(struct ft_timer *, const struct ft_timer_delegate * delegate, struct ft_context * context, ev_tstamp after, ev_tstamp repeat);
void ft_timer_fini(struct ft_timer *);

bool ft_timer_start(struct ft_timer * this);
bool ft_timer_stop(struct ft_timer * this);
bool ft_timer_is_started(struct ft_timer *);

#endif // FT_WATCHER_TIMER_H_
