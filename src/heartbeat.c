#include "_ft_internal.h"

bool ft_heartbeat_init(struct ft_heartbeat * this, struct ft_context * context, ft_heartbeat_cb callback)
{
	assert(this != NULL);
	assert(callback != NULL);

	this->context = context;
	this->callback = callback;
	this->data = NULL;
	this->next = context->on_heartbeat_list;
	context->on_heartbeat_list = this;

	return true;
}


void ft_heartbeat_fini(struct ft_heartbeat * this)
{
	assert(this != NULL);
	assert(this->context != NULL);

	for(struct ft_heartbeat ** p = &this->context->on_heartbeat_list;*p != NULL; p = &(*p)->next)
	{
		if (*p != this) continue;
		*p = this->next;
		goto good;
	}
	FT_WARN("Exit watcher was not found in the context");

good:
	this->context = NULL;
}


void ft_heartbeat_invoke_all_(struct ft_heartbeat * list,  struct ft_context * context, ev_tstamp now)
{
	for (struct ft_heartbeat * this = list; this != NULL; this = this->next)
	{
		this->callback(this, context, now);
	}
}
