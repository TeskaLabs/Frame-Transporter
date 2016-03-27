#include "all.h"

///

struct log_entry
{
	double timestamp;
	pid_t pid;
	char level;
	char message[4096];
};

///

static inline const char * loglevelname(char level)
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

static void log_process(struct log_entry * le, int le_message_length)
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
		loglevelname(le->level),
		le_message_length, le->message
	);

	libsccmn_config.log_flush_counter += 1;
}

static inline int log_format(struct log_entry * le, char level,const char * format, va_list args)
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
	int le_message_length = log_format(&le, level, format, args);
	log_process(&le, le_message_length);
}

void _log_errno_v(int errnum, char level, const char * format, va_list args)
{
	static struct log_entry le;
	int le_message_length = log_format(&le, level, format, args);

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

	log_process(&le, le_message_length);
}

/* TODO: OpenSSL logging integration ...
static int log_openssl_err_print(const char *str, size_t len, void *le_raw)
{
	struct log_entry * le = (struct log_entry *)le_raw;
	int le_message_length = strlen(le->message);
	memcpy(le->message+le_message_length, "                           OpenSSL: ", 36);
	memcpy(le->message+le_message_length+36, str, len);
	return 0;
}

void _log_openssl_err_v(char level, const char * format, va_list args)
{
	static struct log_entry le;
	int le_message_length = log_format(&le, level, format, args);

	if (le_message_length > 0) le.message[le_message_length++] = '\n';
	le.message[le_message_length++] = '\0';

	ERR_print_errors_cb(log_openssl_err_print, &le);
	le_message_length = strlen(le.message);
	while (le.message[le_message_length] == '\0') le_message_length -= 1;
	log_process(&le, le_message_length);
}
*/

///


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


void logging_open_file()
{
	if (libsccmn_config.log_filename == NULL)
	{
		L_WARN("Log file name is not specified");
		return;
	}

	FILE * f = fopen(libsccmn_config.log_filename, "a");
	if (f == NULL)
	{
		L_ERROR_ERRNO(errno, "Error when opening log file '%s'", libsccmn_config.log_filename);
	}

	else
	{
		if (libsccmn_config.log_f != NULL)
			fclose(libsccmn_config.log_f);

		libsccmn_config.log_f = f;
	}
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
