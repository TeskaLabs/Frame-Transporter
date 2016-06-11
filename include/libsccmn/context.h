#ifndef __LIBSCCMN_CONTEXT_H__
#define __LIBSCCMN_CONTEXT_H__

struct context;
struct exiting_watcher;

typedef void (* exiting_cb)(struct exiting_watcher * watcher, struct context * context);

struct exiting_watcher
{
	struct exiting_watcher * next;
	struct exiting_watcher * prev;

	exiting_cb cb;
	void * data;
};

//

struct context
{
	struct ev_loop * ev_loop;

	ev_signal sighup_w;
	ev_signal sigint_w;
	ev_signal sigterm_w;

	struct heartbeat heartbeat;
	struct frame_pool frame_pool;

	struct exiting_watcher * first_exiting_watcher;
	struct exiting_watcher * last_exiting_watcher;
};

bool context_init(struct context * );
void context_fini(struct context * );

void context_evloop_run(struct context * );

void context_exiting_watcher_add(struct context * this, struct exiting_watcher * watcher, exiting_cb cb);
void context_exiting_watcher_remove(struct context * this, struct exiting_watcher * watcher);

#endif //__LIBSCCMN_CONTEXT_H__
