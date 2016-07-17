#ifndef FT_CONTEXT_H_
#define FT_CONTEXT_H_

struct context
{
	struct ev_loop * ev_loop;

	ev_signal sighup_w;
	ev_signal sigint_w;
	ev_signal sigterm_w;

	struct heartbeat heartbeat;
	struct frame_pool frame_pool;

	struct ft_list on_termination_list;
};

bool context_init(struct context * );
void context_fini(struct context * );

void context_evloop_run(struct context * );


typedef void (* ft_context_on_termination_callback)(struct context * context, void * data);

bool ft_context_at_termination(struct context * , ft_context_on_termination_callback callback, void * data);

#endif // FT_CONTEXT_H_
