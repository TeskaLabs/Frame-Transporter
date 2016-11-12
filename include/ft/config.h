#ifndef FT_CONFIG_H_
#define FT_CONFIG_H_

struct ft_config
{
	bool initialized;

	bool log_verbose;
	struct ft_log_backend * log_backend;
	unsigned int log_trace_mask;

	// This part is related to logging to file
	struct
	{
		const char * filename;
		FILE * file;
		ev_signal sighup_w;
		unsigned int flush_counter;
		unsigned int flush_counter_max;
		ev_tstamp flush_last;
		double flush_interval;
		char datetime_style; // 'U' = UTC, 'L' = Local time, 'I' = ISO 8601
	} log_file;

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
