#ifndef FT_CONFIG_H_
#define FT_CONFIG_H_

struct ft_config
{
	bool initialized;

	bool log_verbose;
	unsigned int log_trace_mask;
	const char * log_filename;
	FILE * log_f;
	unsigned int log_flush_counter;
	unsigned int log_flush_counter_max;
	ev_tstamp log_flush_last;
	double log_flush_interval;
	bool log_use_utc;

	double lag_detector_sensitivity;

	char * pid_file;

	unsigned int libev_loop_flags; // See EVFLAG_* in libev

	ev_tstamp fpool_zone_free_timeout;

	ev_tstamp heartbeat_interval;

	int sock_listen_backlog;

	unsigned int sock_est_max_read_loops;
	unsigned int sock_est_max_write_loops;
	int stream_ssl_ex_data_index; // Where to store a pointer to socket object
};

extern struct ft_config ft_config;

#endif // FT_CONFIG_H_
