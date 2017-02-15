#ifndef FT_CONTEXT_H_
#define FT_CONTEXT_H_

struct ft_context
{
	struct ev_loop * ev_loop;

	ev_signal sigint_w;
	ev_signal sigterm_w;
	ev_signal sighup_w;

	ev_timer heartbeat_w;
	ev_tstamp heartbeat_at;

	ev_tstamp started_at;
	ev_tstamp shutdown_at;
	ev_timer shutdown_w;

	ev_prepare prepare_w;

	unsigned int shutdown_counter;

	struct
	{
		bool running:1;
	} flags;

	struct ft_pool frame_pool;

	struct ft_exit * on_exit_list;
	struct ft_heartbeat * on_heartbeat_list;
};

bool ft_context_init(struct ft_context * );
void ft_context_fini(struct ft_context * );

void ft_context_run(struct ft_context * );

// Can be safely called with NULL in the context
ev_tstamp ft_safe_now(struct ft_context *);

#endif // FT_CONTEXT_H_
