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
	.log_use_utc = true,

	.lag_detector_sensitivity = 1.0,

	.pid_file = NULL,

	.libev_loop_flags = 0,

	.fpool_zone_free_timeout = 2.0,

	.heartbeat_interval = 0.1,

	.sock_listen_backlog = 128,
};


void libsccmn_init(void)
{
	assert(libsccmn_config.initialized == false);

	// Initialize logging
	_logging_init();

	// Initialize OpenSSL
	SSL_library_init();

	// Ignore SIGPIPE
	signal(SIGPIPE, SIG_IGN);

	libsccmn_config.initialized = true;

	// Initialize OpenSSL
	if (libsccmn_config.log_verbose)
	{
		SSL_load_error_strings();
		ERR_load_crypto_strings();
	}

	// Print and check versions
	L_DEBUG("Using libev %d.%d", ev_version_major(), ev_version_minor());
	if ((ev_version_major() != EV_VERSION_MAJOR) || (ev_version_minor() < EV_VERSION_MINOR))
	{
		L_ERROR("Incompatible version of libev used (%d.%d != %d.%d)", ev_version_major(), ev_version_minor(), EV_VERSION_MAJOR, EV_VERSION_MINOR);
		exit(EXIT_FAILURE);
	}

	L_DEBUG("Using %s", SSLeay_version(SSLEAY_VERSION));
	if ((SSLeay() ^ OPENSSL_VERSION_NUMBER) & ~0xff0L)
	{
		L_ERROR("Incompatible version of OpenSSL used (%lX [present] != %lX [expected])", SSLeay(), SSLEAY_VERSION_NUMBER);
		exit(EXIT_FAILURE);
	}

	// Re-seed OpenSSL random number generator
	long long seed[4];
	seed[0] = (long long)time(NULL);
	seed[1] = (long long)getpid();
	RAND_seed(seed, sizeof(seed));
}
