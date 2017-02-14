#include "_ft_internal.h"


bool ft_exit_init(struct ft_exit * this, struct ft_context * context, ft_exit_cb callback)
{
	assert(this != NULL);
	assert(callback != NULL);

	this->context = context;
	this->callback = callback;
	this->data = NULL;
	this->next = context->on_exit_list;
	context->on_exit_list = this;

	return true;
}


void ft_exit_fini(struct ft_exit * this)
{
	assert(this != NULL);
	assert(this->context != NULL);

	for(struct ft_exit ** p = &this->context->on_exit_list;*p != NULL; p = &(*p)->next)
	{
		if (*p != this) continue;
		*p = this->next;
		goto good;
	}
	FT_WARN("Exit watcher was not found in the context");

good:
	this->context = NULL;
}


void ft_exit_invoke_all(struct ft_exit * list,  struct ft_context * context, enum ft_exit_phase phase)
{
	for (struct ft_exit * this = list; this != NULL; this = this->next)
	{
		this->callback(this, context, phase);
	}
}
