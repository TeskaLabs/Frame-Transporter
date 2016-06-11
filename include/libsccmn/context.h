#ifndef __LIBSCCMN_CONTEXT_H__
#define __LIBSCCMN_CONTEXT_H__

struct context
{
	struct ev_loop * ev_loop;

	ev_signal sighup_w;
	ev_signal sigint_w;
	ev_signal sigterm_w;

	struct heartbeat heartbeat;
	struct frame_pool frame_pool;
};

bool context_init(struct context * );
void context_fini(struct context * );

void context_evloop_run(struct context * );

#endif //__LIBSCCMN_CONTEXT_H__
