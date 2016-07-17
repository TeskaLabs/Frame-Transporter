#include "_ft_internal.h"

///

static const char * _ft_log_months[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

static inline const char * _ft_log_levelname(char level)
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

///

static struct context * _ft_log_context = NULL;

inline static ev_tstamp _ft_log_get_tstamp()
{
	return ((_ft_log_context != NULL) && (_ft_log_context->ev_loop != NULL)) ? ev_now(_ft_log_context->ev_loop) : ev_time();
}

///

void ft_logrecord_process(struct ft_logrecord * le, int le_message_length)
{
	time_t t = le->timestamp;
	struct tm tmp;
	if (ft_config.log_use_utc)
		gmtime_r(&t, &tmp);
	else
		localtime_r(&t, &tmp);
	unsigned int frac100 = (le->timestamp * 1000) - (t * 1000);

	fprintf(ft_config.log_f != NULL ? ft_config.log_f : stderr, 
		"%s %02d %04d %02d:%02d:%02d.%03d %s %7d %s: %.*s\n",
		_ft_log_months[tmp.tm_mon], tmp.tm_mday, 1900+tmp.tm_year,
		tmp.tm_hour, tmp.tm_min, tmp.tm_sec, frac100,
		tmp.tm_zone,
		le->pid,
		_ft_log_levelname(le->level),
		le_message_length, le->message
	);

	ft_config.log_flush_counter += 1;
}

static inline int _ft_logrecord_build(struct ft_logrecord * le, char level,const char * format, va_list args)
{
	le->timestamp = _ft_log_get_tstamp();
	le->pid = getpid();
	le->level = level;

	if (format != NULL) return vsnprintf(le->message, sizeof(le->message), format, args);

	le->message[0] = '\0';
	return 0;
}

///

void _ft_log_v(const char level, const char * format, va_list args)
{
	static struct ft_logrecord le;
	int le_message_length = _ft_logrecord_build(&le, level, format, args);
	ft_logrecord_process(&le, le_message_length);
}

void _ft_log_errno_v(int errnum, const char level, const char * format, va_list args)
{
	static struct ft_logrecord le;
	int le_message_length = _ft_logrecord_build(&le, level, format, args);

	if (le_message_length > 0)
	{
		le.message[le_message_length++] = ':';
		le.message[le_message_length++] = ' ';
	}
	le.message[le_message_length] = '\0';

#ifdef _GNU_SOURCE
	// The GNU-specific strerror_r() is in use ...
	char * im = strerror_r(errnum, le.message + le_message_length, sizeof(le.message)-le_message_length);
	if (im != (le.message + le_message_length)) strcpy(le.message + le_message_length, im);
	le_message_length += strlen(im);
#else
	strerror_r(errnum, le.message + le_message_length, sizeof(le.message)-le_message_length);
	le_message_length += strlen(le.message + le_message_length);
#endif

	le_message_length += snprintf(le.message + le_message_length, sizeof(le.message)-le_message_length, " (%d)", errnum);

	ft_logrecord_process(&le, le_message_length);
}

void _ft_log_openssl_err_v(const char level, const char * format, va_list args)
{
	static struct ft_logrecord le;
	int le_message_length = _ft_logrecord_build(&le, level, format, args);

	unsigned long es = CRYPTO_thread_id();
	while (true)
	{
		const char * file;
		int line;
		const char * data;
		int flags;
		unsigned long code = ERR_get_error_line_data(&file, &line, &data, &flags);
		if (code == 0) break;

		char buf[256];
		ERR_error_string_n(code, buf, sizeof(buf));

		int len = snprintf(le.message+le_message_length, sizeof(le.message) - (le_message_length+1), 
			"\n                                       SSL: %lu:%s:%s:%d:%s",
			es, buf, file, line, (flags & ERR_TXT_STRING) ? data : ""
		);
		le_message_length += len;
	}

	ft_logrecord_process(&le, le_message_length);
}


static void _ft_log_libev_on_syserr(const char * msg)
{
	FT_WARN("LibEv: %s", msg);
}

///

void _ft_log_initialise()
{
	ev_set_syserr_cb(_ft_log_libev_on_syserr);
}


void ft_log_flush()
{
	ev_tstamp now = _ft_log_get_tstamp();
	bool do_flush = false;

	if (ft_config.log_flush_counter >= ft_config.log_flush_counter_max)
	{
		do_flush = true;
	}
	else if ((ft_config.log_flush_last + ft_config.log_flush_interval) < now)
	{
		// Log entries are not that old to initiale the flush
		do_flush = true;
	}	

	if (!do_flush) return;

	fflush(ft_config.log_f != NULL ? ft_config.log_f : stderr);
	ft_config.log_flush_counter = 0;
	ft_config.log_flush_last = now;
}


bool ft_log_filename(const char * fname)
{
	if (ft_config.log_filename != NULL)
	{
		ft_log_finalise();
		free((void *)ft_config.log_filename);
		ft_config.log_filename = NULL;
	}

	ft_config.log_filename = (fname != NULL) ? strdup(fname) : NULL;
	return ft_log_reopen();
}


bool ft_log_reopen()
{
	if ((ft_config.log_filename == NULL) && (ft_config.log_f == NULL))
	{
		// No-op when filename and file is not specified 
		return true;
	}

	if (ft_config.log_filename == NULL)
	{
		FT_WARN("Log file name is not specified");
		return false;
	}

	FILE * f = fopen(ft_config.log_filename, "a");
	if (f == NULL)
	{
		FT_ERROR_ERRNO(errno, "Error when opening log file '%s'", ft_config.log_filename);
		return false;
	}

	bool reopen = (ft_config.log_f != NULL);
	if (reopen)
	{
		FT_INFO("Log file is closed");
		fclose(ft_config.log_f);
	}

	ft_config.log_f = f;
	FT_INFO("Log file is %s", reopen ? "reopen" : "open");

	return true;
}


void ft_log_finalise()
{
	if (ft_config.log_f != NULL)
	{
		fflush(ft_config.log_f);
		fclose(ft_config.log_f);
		ft_config.log_f = NULL;
	}

	fflush(stderr);
}

///


void ft_log_context(struct context * context)
{
	if (context == NULL)
	{
		assert(_ft_log_context != NULL);
		_ft_log_context = NULL;
		return;
	}

	assert(_ft_log_context == NULL);
	_ft_log_context = context;
}
