#include "all.h"

struct libsccmn_config libsccmn_config =
{
	.initialized = false, // Will be changed to true when initialized

	.log_verbose = false,
	.log_filename = NULL,
	.log_f = NULL,
	.log_flush_counter = 0,
	.log_flush_counter_max = 100,
	.log_flush_last = 0.0,
	.log_flush_interval = 5.0, //Seconds

	.ev_loop = NULL,
};


void libsccmn_config_init()
{
	assert(libsccmn_config.initialized == false);


	libsccmn_config.initialized = true;
}
