#include "../_ft_internal.h"

double ft_time()
{
	
	return ev_time();
}


double ft_now(struct ft_context * context)
{
	return ev_now(context->ev_loop);
}


double ft_safe_now(struct ft_context * this)
{
	return ((this != NULL) && (this->ev_loop != NULL)) ? ft_now(this) : ft_time();
}
