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
	struct ft_pubsub pubsub;
};

bool ft_context_init(struct ft_context * );
void ft_context_fini(struct ft_context * );

void ft_context_run(struct ft_context * );

// This is a polite way of termination
// It doesn't guarantee that event loop is stopped, there can be a rogue watcher still running
void ft_context_stop(struct ft_context * );

// Default pool (will be set by first ft_context_init() call ... or you can override it)
// It removes need for complicated context localisation in the code
extern struct ft_context * ft_context_default;

#endif // FT_CONTEXT_H_
