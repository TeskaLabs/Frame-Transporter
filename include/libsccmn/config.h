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

	struct ev_loop * ev_loop;
};

extern struct libsccmn_config libsccmn_config;

void libsccmn_config_init(void);

#endif // __LIBSCCMN_CONFIG_H__
