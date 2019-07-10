#include "../_ft_internal.h"

ev_tstamp ft_time()
{
	
	return ev_time();
}


ev_tstamp ft_now(struct ft_context * context)
{
	return ev_now(context->ev_loop);
}


ev_tstamp ft_safe_now(struct ft_context * this)
{
	return ((this != NULL) && (this->ev_loop != NULL)) ? ft_now(this) : ft_time();
}
