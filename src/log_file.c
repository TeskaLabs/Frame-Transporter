#include "_ft_internal.h"

///

static const char * _ft_log_file_months[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

static inline const char * _ft_log_file_levelname(char level)
{
	static const char * lln_TRACE = "TRACE";
	static const char * lln_DEBUG = "DEBUG";
	static const char * lln_INFO = " INFO";
	static const char * lln_WARN = " WARN";
	static const char * lln_ERROR = "ERROR";
	static const char * lln_FATAL = "FATAL";
	static const char * lln_AUDIT = "AUDIT";
	static const char * lln_UNKNOWN= "?????";

	switch (level)
	{
		case 'T': return lln_TRACE;
		case 'D': return lln_DEBUG;
		case 'I': return lln_INFO;
		case 'W': return lln_WARN;
		case 'E': return lln_ERROR;
		case 'F': return lln_FATAL;
		case 'A': return lln_AUDIT;
	}

	return lln_UNKNOWN;
}

static void ft_log_file_fini()
{
	if (ft_config.log_file.file != NULL)
	{
		fclose(ft_config.log_file.file);
		ft_config.log_file.file = NULL;
	}
}


static void ft_log_file_logrecord_process(struct ft_logrecord * le, int le_message_length)
{
	time_t t = le->timestamp;
	struct tm tmp;
	if (ft_config.log_file.datetime_style == 'L')
	{
		localtime_r(&t, &tmp);
	}
	else
	{
		gmtime_r(&t, &tmp);
	}
	unsigned int frac100 = (le->timestamp * 1000) - (t * 1000);

	if (ft_config.log_file.datetime_style == 'I')
	{
		// ISO 8601
		fprintf(ft_config.log_file.file != NULL ? ft_config.log_file.file : stderr, 
			"%04d-%02d-%02dT%02d:%02d:%02d.%03dZ%6d %s: %.*s\n",
			1900+tmp.tm_year, tmp.tm_mon, tmp.tm_mday,
			tmp.tm_hour, tmp.tm_min, tmp.tm_sec, frac100,
			le->pid,
			_ft_log_file_levelname(le->level),
			le_message_length, le->message
		);
	} else {
		fprintf(ft_config.log_file.file != NULL ? ft_config.log_file.file : stderr, 
			"%s %02d %04d %02d:%02d:%02d.%03d %s %6d %s: %.*s\n",
			_ft_log_file_months[tmp.tm_mon], tmp.tm_mday, 1900+tmp.tm_year,
			tmp.tm_hour, tmp.tm_min, tmp.tm_sec, frac100,
			tmp.tm_zone,
			le->pid,
			_ft_log_file_levelname(le->level),
			le_message_length, le->message
		);
	}

	ft_config.log_file.flush_counter += 1;
}

static void ft_log_file_on_heartbeat(struct ft_context * context, ev_tstamp now)
{
	ft_log_file_flush(now);
}

struct ft_log_backend ft_log_file_backend = {
	.fini = ft_log_file_fini,
	.heartbeat = ft_log_file_on_heartbeat,
	.process = ft_log_file_logrecord_process,
};


void ft_log_file_flush(ev_tstamp now)
{
	bool do_flush = false;

	if (ft_config.log_file.flush_counter >= ft_config.log_file.flush_counter_max)
	{
		do_flush = true;
	}
	else if ((ft_config.log_file.flush_last + ft_config.log_file.flush_interval) < now)
	{
		do_flush = true;
	}	

	if (!do_flush) return;

	fflush(ft_config.log_file.file != NULL ? ft_config.log_file.file : stderr);
	ft_config.log_file.flush_counter = 0;
	ft_config.log_file.flush_last = now;
}


bool ft_log_file_reopen()
{
	if ((ft_config.log_file.filename == NULL) && (ft_config.log_file.file == NULL))
	{
		// No-op when filename and file is not specified 
		return true;
	}

	if (ft_config.log_file.filename == NULL)
	{
		FT_WARN("Log file name is not specified");
		return false;
	}

	FILE * f = fopen(ft_config.log_file.filename, "a");
	if (f == NULL)
	{
		FT_ERROR_ERRNO(errno, "Error when opening log file '%s'", ft_config.log_file.filename);
		return false;
	}

	bool reopen = (ft_config.log_file.file != NULL);
	if (reopen)
	{
		FT_INFO("Log file is closed");
		fclose(ft_config.log_file.file);
	}

	ft_config.log_file.file = f;
	FT_INFO("Log file is %s", reopen ? "reopen" : "open");

	return true;
}

static void ft_log_file_on_sighup(struct ev_loop * loop, ev_signal * w, int revents)
{
	ft_log_file_reopen();
}


bool ft_log_file_set(const char * fname)
{
	if (ft_config.log_file.filename != NULL)
	{
		ft_log_file_fini();
		free((void *)ft_config.log_file.filename);
		ft_config.log_file.filename = NULL;
	}

	ft_config.log_file.filename = (fname != NULL) ? strdup(fname) : NULL;
	return ft_log_file_reopen();
}


void ft_log_handle_sighup(struct ft_context * context)
{
	assert(context != NULL);

	if (ft_config.log_file.sighup_w.signum != 0)
	{
		FT_WARN("SIGHUP handler seems to be already installed");
		return;
	}

	ev_signal_init(&ft_config.log_file.sighup_w, ft_log_file_on_sighup, SIGHUP);
	ev_signal_start(context->ev_loop, &ft_config.log_file.sighup_w);
	ev_unref(context->ev_loop);
}
