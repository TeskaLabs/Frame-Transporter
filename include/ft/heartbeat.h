#ifndef FT_HEARTBEAT_H_
#define FT_HEARTBEAT_H_

struct heartbeat;
struct heartbeat_watcher;

typedef void (* heartbeat_cb)(struct heartbeat_watcher * watcher, struct heartbeat * heartbeat, ev_tstamp now);

struct heartbeat_watcher
{
	struct heartbeat_watcher * next;
	struct heartbeat_watcher * prev;

	heartbeat_cb cb;
	void * data;
};

struct heartbeat
{
	struct ft_context * context;

	struct heartbeat_watcher * first_watcher;
	struct heartbeat_watcher * last_watcher;

	ev_timer timer_w;
	ev_tstamp last_beat;
};

void heartbeat_init(struct heartbeat * , struct ft_context * context);

void heartbeat_start(struct heartbeat *);
void heartbeat_stop(struct heartbeat *);

void heartbeat_add(struct heartbeat *, struct heartbeat_watcher * watcher, heartbeat_cb cb);
void heartbeat_remove(struct heartbeat *, struct heartbeat_watcher * watcher);

#endif // FT_HEARTBEAT_H_
