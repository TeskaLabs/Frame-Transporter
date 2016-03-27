#include "all.h"

struct libsccmn_config libsccmn_config =
{
	.initialized = false, // Will be changed to true when initialized

	.ev_loop = NULL,
};


void libsccmn_config_init()
{
	assert(libsccmn_config.initialized == false);


	libsccmn_config.initialized = true;
}
