#ifndef __LIBSCCMN_CONFIG_H__
#define __LIBSCCMN_CONFIG_H__

struct libsccmn_config
{
	bool initialized;

	bool log_verbose;
	const char * log_filename;
	FILE * log_f;
	unsigned int log_flush_counter;
	unsigned int log_flush_counter_max;
	ev_tstamp log_flush_last;
	double log_flush_interval;
	ev_signal log_reopen_sighup_w;
	bool log_use_utc;

	struct ev_loop * ev_loop;
	double lag_detector_sensitivity;

	char * pid_file;

	ev_tstamp fpool_zone_free_timeout;
};

extern struct libsccmn_config libsccmn_config;

#endif // __LIBSCCMN_CONFIG_H__
