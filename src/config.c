#include "_ft_internal.h"

struct ft_config ft_config =
{
	.initialized = false, // Will be changed to true when initialized

	.log_verbose = false,
	.log_backend = &ft_log_file_backend,
	.log_trace_mask = 0,

	.log_file = {
		.filename = NULL,
		.file = NULL,
		.flush_counter = 0,
		.flush_counter_max = 100,
		.flush_last = 0.0,
		.flush_interval = 5.0, //Seconds
		.datetime_style = 'U',
	},

	.lag_detector_sensitivity = 1.0,

	.pid_file = NULL,

	.libev_loop_flags = 0,

	.fpool_zone_free_timeout = 2.0,

	.heartbeat_interval = 2.0,

	.sock_listen_backlog = 128,

	.sock_est_max_read_loops = 10,
	.sock_est_max_write_loops = 10,
	.stream_ssl_ex_data_index = -2,
};


void ft_initialise(void)
{
	assert(ft_config.initialized == false);

	// Initialize logging
	ft_log_initialise_();

	// Initialize OpenSSL
	SSL_library_init();

	// Ignore SIGPIPE
	signal(SIGPIPE, SIG_IGN);

	ft_config.initialized = true;

	// Initialize OpenSSL
	if (ft_config.log_verbose)
	{
		SSL_load_error_strings();
		ERR_load_crypto_strings();
	}

	// Print and check versions
	if ((ev_version_major() != EV_VERSION_MAJOR) || (ev_version_minor() < EV_VERSION_MINOR))
	{
		FT_ERROR("Incompatible version of libev used (%d.%d != %d.%d)", ev_version_major(), ev_version_minor(), EV_VERSION_MAJOR, EV_VERSION_MINOR);
		exit(EXIT_FAILURE);
	}

	if ((SSLeay() ^ OPENSSL_VERSION_NUMBER) & ~0xff0L)
	{
		FT_ERROR("Incompatible version of OpenSSL used (%lX [present] != %lX [expected])", SSLeay(), SSLEAY_VERSION_NUMBER);
		exit(EXIT_FAILURE);
	}

	// Re-seed OpenSSL random number generator
	long long seed[4];
	seed[0] = (long long)time(NULL);
	seed[1] = (long long)getpid();
	RAND_seed(seed, sizeof(seed));
}
