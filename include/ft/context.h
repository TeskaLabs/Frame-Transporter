#ifndef FT_CONTEXT_H_
#define FT_CONTEXT_H_

struct ft_context
{
	struct ev_loop * ev_loop;

	ev_signal sighup_w;
	ev_signal sigint_w;
	ev_signal sigterm_w;

	ev_timer heartbeat_w;
	ev_tstamp heartbeat_at;

	ev_tstamp started_at;
	ev_tstamp shutdown_at;
	ev_timer shutdown_w;

	unsigned int shutdown_counter;

	struct
	{
		bool running:1;
	} flags;

	struct ft_pool frame_pool;

	struct ft_list on_termination_list;
	struct ft_list on_heartbeat_list;
};

bool ft_context_init(struct ft_context * );
void ft_context_fini(struct ft_context * );

void ft_context_run(struct ft_context * );


typedef void (* ft_context_callback)(struct ft_context * context, void * data);

bool ft_context_at_termination(struct ft_context * , ft_context_callback callback, void * data);
bool ft_context_at_heartbeat(struct ft_context * , ft_context_callback callback, void * data);

#endif // FT_CONTEXT_H_
