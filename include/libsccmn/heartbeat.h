#ifndef __LIBSCCMN_HEARTBEAT_H__
#define __LIBSCCMN_HEARTBEAT_H__

typedef void (* heartbeat_cb)(struct ev_loop * loop, ev_tstamp now);

struct heartbeat
{
	size_t size;
	int top;
	heartbeat_cb (*cbs)[];

	ev_timer timer_w;
};

bool heartbeat_init(struct heartbeat * , size_t size, ev_tstamp repeat);
bool heartbeat_fini(struct heartbeat *);

bool heartbeat_start(struct ev_loop * loop, struct heartbeat *);
bool heartbeat_stop(struct ev_loop * loop, struct heartbeat *);

bool heartbeat_add(struct heartbeat *, heartbeat_cb cb);

#endif //__LIBSCCMN_HEARTBEAT_H__

