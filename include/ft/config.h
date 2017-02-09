#ifndef FT_CONFIG_H_
#define FT_CONFIG_H_

struct ft_config
{
	bool initialized;

	char appname[32];

	bool log_verbose;
	struct ft_log_backend * log_backend;
	unsigned int log_trace_mask;

	// This part is related to logging to file
	struct
	{
		const char * filename;
		FILE * file;
		unsigned int flush_counter;
		unsigned int flush_counter_max;
		ev_tstamp flush_last;
		double flush_interval;
		char datetime_style; // 'U' = UTC, 'L' = Local time, 'I' = ISO 8601
	} log_file;

	struct
	{
		const char * address;
		int facility;
		char format; // Old BSD-Style (RFC3164) '3', best-effort 'C' or RFC5424 '5'
		char hostname[256];
		char domainname[256];
	} log_syslog;

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
