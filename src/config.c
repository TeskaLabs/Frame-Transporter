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

	.pid_file = NULL,
};


void libsccmn_initialize(void)
{
	assert(libsccmn_config.initialized == false);

	// Check used version of libev
	assert(ev_version_major() == EV_VERSION_MAJOR);
	assert(ev_version_minor() >= EV_VERSION_MINOR);

	// Initialize logging
	_logging_init();

	SSL_library_init();


	libsccmn_config.initialized = true;
}

void libsccmn_configure(void)
{
	assert(libsccmn_config.initialized == true);

	// Initialize OpenSSL
	if (libsccmn_config.log_verbose)
	{
		SSL_load_error_strings();
		ERR_load_crypto_strings();
	}

}
