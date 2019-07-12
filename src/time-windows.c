#include "_ft_internal.h"

double ft_time()
{
	FILETIME ft;
	ULARGE_INTEGER ui;

	GetSystemTimeAsFileTime(&ft);
	ui.u.LowPart  = ft.dwLowDateTime;
	ui.u.HighPart = ft.dwHighDateTime;

	double now = (ui.QuadPart - 116444736000000000) * 1e-7;
	return now;
}


double ft_now(struct ft_context * context)
{
	return context->now;
}


double ft_safe_now(struct ft_context * this)
{
	return (this != NULL) ? ft_now(this) : ft_time();
}
