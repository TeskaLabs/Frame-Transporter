#ifndef FT_CONTEXT_H_
#define FT_CONTEXT_H_

struct ft_context
{
	// struct ev_loop * ev_loop;

	// ev_signal sigint_w;
	// ev_signal sigterm_w;
	// ev_signal sighup_w;
	// ev_prepare prepare_w;

	struct ft_timer heartbeat_w;
	ev_tstamp heartbeat_at;

	ev_tstamp started_at;
	ev_tstamp shutdown_at;
	struct ft_timer shutdown_w;

	unsigned int shutdown_counter;

	struct
	{
		bool running:1;
	} flags;

	struct ft_pool frame_pool;
	struct ft_pubsub pubsub;

	int refs;
	ev_tstamp now;

	struct ft_watcher * active_watchers;
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

// Add/remove watcher from an active list
void ft_context_watcher_add(struct ft_context * , struct ft_watcher *);
void ft_context_watcher_remove(struct ft_context * , struct ft_watcher *);
bool ft_context_watcher_is_added(struct ft_context * , struct ft_watcher *);


static inline void ft_unref(struct ft_context * context) {
	assert(context != NULL);
	context->refs -= 1;
}

static inline void ft_ref(struct ft_context * context) {
	assert(context != NULL);
	context->refs += 1;
}

#endif // FT_CONTEXT_H_
