#ifndef FT_TIMER_H_
#define FT_TIMER_H_

struct ft_timer;

struct ft_timer_delegate
{
	void (*tick)(struct ft_timer *); // Mandatory
	void (*fini)(struct ft_timer *); // Optional
};


struct ft_timer {
	struct ft_context * context;
	struct ft_timer * next;

	const struct ft_timer_delegate * delegate;

	double after;
	double repeat;

	void * data;
};

bool ft_timer_init(struct ft_timer *, const struct ft_timer_delegate * delegate, struct ft_context * context, double after, double repeat);
void ft_timer_fini(struct ft_timer *);

bool ft_timer_start(struct ft_timer *);
bool ft_timer_stop(struct ft_timer *);
bool ft_timer_is_started(struct ft_timer *);

void ft_timer_again(struct ft_timer *);

double ft_timers_process(struct ft_context *, unsigned int * trigger_counter);

#endif // FT_TIMER_H_
