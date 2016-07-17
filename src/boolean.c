#include "_ft_internal.h"

bool ft_parse_bool(const char * value)
{
	if (strcasecmp(value, "yes") == 0) return true;
	if (strcasecmp(value, "true") == 0) return true;
	if (strcasecmp(value, "1") == 0) return true;

	if (strcasecmp(value, "no") == 0) return false;
	if (strcasecmp(value, "false") == 0) return false;
	if (strcasecmp(value, "0") == 0) return false;

	FT_WARN("Invalid boolean value: '%s'", value);
	return false;
}
