#include "all.h"

///

static inline const char * log_levelname(char level)
{
	static const char * lln_DEBUG = "DEBUG";
	static const char * lln_INFO = " INFO";
	static const char * lln_WARN = " WARN";
	static const char * lln_ERROR = "ERROR";
	static const char * lln_FATAL = "FATAL";
	static const char * lln_AUDIT = "AUDIT";
	static const char * lln_UNKNOWN= "?????";

	switch (level)
	{
		case 'D': return lln_DEBUG;
		case 'I': return lln_INFO;
		case 'W': return lln_WARN;
		case 'E': return lln_ERROR;
		case 'F': return lln_FATAL;
		case 'A': return lln_AUDIT;
	}

	return lln_UNKNOWN;
}

void log_entry_process(struct log_entry * le, int le_message_length)
{
	time_t t = le->timestamp;
	struct tm tmp;
	localtime_r(&t, &tmp);
	unsigned int frac100 = (le->timestamp * 1000) - (t * 1000);
	char strftime_buf[20];
	strftime(strftime_buf, sizeof(strftime_buf) - 1, "%d%b%y %H:%M:%S", &tmp);

	fprintf(libsccmn_config.log_f != NULL ? libsccmn_config.log_f : stderr, 
		"%s.%03d %7d %s: %.*s\n",
		strftime_buf, frac100,
		le->pid,
		log_levelname(le->level),
		le_message_length, le->message
	);

	libsccmn_config.log_flush_counter += 1;
}

static inline int log_entry_format(struct log_entry * le, char level,const char * format, va_list args)
{
	le->timestamp = (libsccmn_config.ev_loop == NULL) ? ev_time() : ev_now(libsccmn_config.ev_loop);
	le->pid = getpid();
	le->level = level;

	if (format != NULL) return vsnprintf(le->message, sizeof(le->message), format, args);

	le->message[0] = '\0';
	return 0;
}

///

void _log_v(char level, const char * format, va_list args)
{
	static struct log_entry le;
	int le_message_length = log_entry_format(&le, level, format, args);
	log_entry_process(&le, le_message_length);
}

void _log_errno_v(int errnum, char level, const char * format, va_list args)
{
	static struct log_entry le;
	int le_message_length = log_entry_format(&le, level, format, args);

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

	log_entry_process(&le, le_message_length);
}

void _log_openssl_err_v(char level, const char * format, va_list args)
{
	static struct log_entry le;
	int le_message_length = log_entry_format(&le, level, format, args);

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
			"\n                               SSL: %lu:%s:%s:%d:%s",
			es, buf, file, line, (flags & ERR_TXT_STRING) ? data : ""
		);
		le_message_length += len;
	}

	log_entry_process(&le, le_message_length);
}


static void logging_libev_on_syserr(const char * msg)
{
	L_WARN("LibEv: %s", msg);
}

///

void _logging_init()
{
	ev_set_syserr_cb(logging_libev_on_syserr);
}


void logging_flush()
{
	ev_tstamp now = (libsccmn_config.ev_loop == NULL) ? ev_time() : ev_now(libsccmn_config.ev_loop);
	bool do_flush = false;

	if (libsccmn_config.log_flush_counter >= libsccmn_config.log_flush_counter_max)
	{
		do_flush = true;
	}
	else if ((libsccmn_config.log_flush_last + libsccmn_config.log_flush_interval) < now)
	{
		// Log entries are not that old to initiale the flush
		do_flush = true;
	}	

	if (!do_flush) return;

	fflush(libsccmn_config.log_f != NULL ? libsccmn_config.log_f : stderr);
	libsccmn_config.log_flush_counter = 0;
	libsccmn_config.log_flush_last = now;
}


bool logging_set_filename(const char * fname)
{
	if (libsccmn_config.log_filename != NULL)
	{
		logging_finish();
		free((void *)libsccmn_config.log_filename);
		libsccmn_config.log_filename = NULL;
	}

	libsccmn_config.log_filename = (fname != NULL) ? strdup(fname) : NULL;
	return logging_reopen();
}


bool logging_reopen()
{
	if ((libsccmn_config.log_filename == NULL) && (libsccmn_config.log_f == NULL))
	{
		// No-op when filename and file is not specified 
		return true;
	}

	if (libsccmn_config.log_filename == NULL)
	{
		L_WARN("Log file name is not specified");
		return false;
	}

	FILE * f = fopen(libsccmn_config.log_filename, "a");
	if (f == NULL)
	{
		L_ERROR_ERRNO(errno, "Error when opening log file '%s'", libsccmn_config.log_filename);
		return false;
	}

	bool reopen = (libsccmn_config.log_f != NULL);
	if (reopen)
	{
		L_INFO("Log file is closed");
		fclose(libsccmn_config.log_f);
	}

	libsccmn_config.log_f = f;
	L_INFO("Log file is %s", reopen ? "reopen" : "open");

	return true;
}


void logging_finish()
{
	if (libsccmn_config.log_f != NULL)
	{
		fflush(libsccmn_config.log_f);
		fclose(libsccmn_config.log_f);
		libsccmn_config.log_f = NULL;
	}

	fflush(stderr);
}

///

static void logging_reopen_on_hup_signal(struct ev_loop * loop, ev_signal * w, int revents)
{
	logging_reopen();
}

void logging_install_sighup_reopen()
{
	ev_signal_init(&libsccmn_config.log_reopen_sighup_w, logging_reopen_on_hup_signal, SIGHUP);
	ev_signal_start(libsccmn_config.ev_loop, &libsccmn_config.log_reopen_sighup_w);
	ev_unref(libsccmn_config.ev_loop);
}
