#include "_ft_internal.h"

ev_tstamp ft_time()
{
	FILETIME ft;
	ULARGE_INTEGER ui;

	GetSystemTimeAsFileTime(&ft);
	ui.u.LowPart  = ft.dwLowDateTime;
	ui.u.HighPart = ft.dwHighDateTime;

	return (ui.QuadPart - 116444736000000000) * 1e-7;
}


ev_tstamp ft_now(struct ft_context * context)
{
	return context->now;
}


ev_tstamp ft_safe_now(struct ft_context * this)
{
	return (this != NULL) ? ft_now(this) : ft_time();
}
