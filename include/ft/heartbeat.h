#ifndef FT_HEARTBEAT_H_
#define FT_HEARTBEAT_H_

// Implementation of heartbeat watcher

struct ft_heartbeat;
typedef void (*ft_heartbeat_cb)(struct ft_heartbeat *, struct ft_context * context, ev_tstamp now);

struct ft_heartbeat
{
	struct ft_context * context;
	struct ft_heartbeat * next;

	ft_heartbeat_cb callback;
	void * data;
};

bool ft_heartbeat_init(struct ft_heartbeat * , struct ft_context * context, ft_heartbeat_cb callback);
void ft_heartbeat_fini(struct ft_heartbeat *);

#endif // FT_HEARTBEAT_H_
