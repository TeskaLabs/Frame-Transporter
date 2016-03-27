#include "all.h"

struct libsccmn_config libsccmn_config =
{
	.initialized = false, // Will be changed to true when initialized
};


void libsccmn_config_init()
{
	assert(libsccmn_config.initialized == false);


	libsccmn_config.initialized = true;
}
