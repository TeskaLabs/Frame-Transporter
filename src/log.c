#include "_ft_internal.h"

///

struct ft_log_stats ft_log_stats = {
	.trace_count = 0,
	.debug_count = 0,
	.info_count = 0,
	.warn_count = 0,
	.error_count = 0,
	.fatal_count = 0,
	.audit_count = 0,

	.reentry_fails = 0,
};

static struct ft_context * ft_log_context_ = NULL;

///

static inline int _ft_logrecord_build(struct ft_logrecord * le, char level, const struct ft_log_sd sd[], const char * format, va_list args)
{
	le->timestamp = ft_safe_now(ft_log_context_);
	le->pid = getpid();
	le->level = level;
	le->appname = ft_config.appname;
	le->sd = sd;
	if (format != NULL) return vsnprintf(le->message, sizeof(le->message), format, args);

	le->message[0] = '\0';
	return 0;
}

///

void _ft_log_v(const char level, const struct ft_log_sd sd[], const char * format, va_list args)
{
	static struct ft_logrecord le;
	int le_message_length = _ft_logrecord_build(&le, level, sd, format, args);
	ft_logrecord_process(&le, le_message_length);
}

void _ft_log_errno_v(int errnum, const char level, const struct ft_log_sd sd[], const char * format, va_list args)
{
	struct ft_logrecord le;

	char errnum_str[16];
	snprintf(errnum_str, sizeof(errnum_str)-1, "%d", errnum);

	char errtxt_buf[1024];	

#ifdef _GNU_SOURCE
	char * errtxt_pstr = strerror_r(errnum, errtxt_buf, sizeof(errtxt_buf));
#else
	strerror_r(errnum, errtxt_buf, sizeof(errtxt_buf));
	char * errtxt_pstr = errtxt_buf;
#endif

	int sd_n = 4;
	//TODO: Expand sd_combined by error codes
	struct ft_log_sd sd_combined[sd_n];

	sd_combined[0] = (struct ft_log_sd){"es", "s"};
	sd_combined[1] = (struct ft_log_sd){"e", errnum_str};
	sd_combined[2] = (struct ft_log_sd){"E", errtxt_pstr};
	sd_combined[sd_n-1] = (struct ft_log_sd){NULL, NULL};

	int le_message_length = _ft_logrecord_build(&le, level, sd_combined, format, args);
	ft_logrecord_process(&le, le_message_length);
}

void _ft_log_openssl_err_v(const char level, const struct ft_log_sd sd[], const char * format, va_list args)
{
	static struct ft_logrecord le;

	unsigned long code = ERR_peek_error();

	char errnum_str[16];
	snprintf(errnum_str, sizeof(errnum_str)-1, "%lu", code);

	char errtxt_str[1024];
	ERR_error_string_n(code, errtxt_str, sizeof(errtxt_str));

	int sd_n = 4;
	//TODO: Expand sd_combined by error codes
	struct ft_log_sd sd_combined[sd_n];

	sd_combined[0] = (struct ft_log_sd){"es", "O"};
	sd_combined[1] = (struct ft_log_sd){"e", errnum_str};
	sd_combined[2] = (struct ft_log_sd){"E", errtxt_str};
	sd_combined[sd_n-1] = (struct ft_log_sd){NULL, NULL};

	int le_message_length = _ft_logrecord_build(&le, level, sd_combined, format, args);

	unsigned long es = CRYPTO_thread_id();
	while (true)
	{
		const char * file;
		int line;
		const char * data;
		int flags;
		code = ERR_get_error_line_data(&file, &line, &data, &flags);
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
	// See ERROR HANDLING in libev
	FT_FATAL("%s", msg);
	abort();
}

///

void ft_log_initialise_()
{
	// Obtain an application name
#if defined(__APPLE__) || defined(__FreeBSD__)
	const char * appname = getprogname();
#elif defined(_GNU_SOURCE)
	const char * appname = program_invocation_name;
#else
	const char * appname = "?";
#endif

	const char * p = strrchr(appname, '/');
	if (p == NULL) p = appname;
	else p += 1;
	strncpy(ft_config.appname, p, sizeof(ft_config.appname)-1);
	ft_config.appname[sizeof(ft_config.appname)-1] = '\0';

	ev_set_syserr_cb(_ft_log_libev_on_syserr);
}

void ft_log_finalise()
{
	struct ft_log_backend * log_backend = ft_config.log_backend;

	// Switch to slow log
	ft_config.log_backend = NULL;
	if (ft_log_context_ != NULL) ft_log_context(NULL);

	if ((log_backend != NULL) && (log_backend->fini != NULL))
	{
		log_backend->fini();
	}

	fflush(stderr);
}


void ft_log_flush(bool force)
{
	ev_tstamp now;
	if (force) now = 1e77;
	else
	{
		if (ft_log_context_ == NULL) now = 1e77;
		else now = ev_now(ft_log_context_->ev_loop);
	}

	if ((ft_config.log_backend != NULL) && (ft_config.log_backend->flush != NULL))
		ft_config.log_backend->flush(now);
}


void ft_log_context(struct ft_context * context)
{
	if (context == NULL)
	{
		assert(ft_log_context_ != NULL);
		ft_log_context_ = NULL;
		return;
	}

	assert(ft_log_context_ == NULL);
	ft_log_context_ = context;
}


void ft_log_backend_switch(struct ft_log_backend * backend)
{
	struct ft_log_backend * old_backend = ft_config.log_backend;
	if (old_backend == backend) return;

	if (backend == NULL)
	{
		backend = &ft_log_file_backend;
	}

	ft_config.log_backend = backend;

	if ((old_backend != NULL) && (old_backend->fini != NULL))
		old_backend->fini();
}


void ft_logrecord_process(struct ft_logrecord * le, int le_message_length)
{
	assert(le_message_length >= 0);

	if ((ft_config.log_backend == NULL) || (ft_config.log_backend->process == NULL))
	{
		// No log backend configured
		ft_logrecord_emergency_fprint(le, le_message_length, stderr);
		return;
	}

	static bool ft_logrecord_process_reentry = false;
	if (ft_logrecord_process_reentry == true)
	{
		ft_log_stats.reentry_fails += 1;
		// Log record re-entry ... that's an emergency situation
		ft_logrecord_emergency_fprint(le, le_message_length, stderr);
		return;
	}
	ft_logrecord_process_reentry = true;

	ft_config.log_backend->process(le, le_message_length);

	ft_logrecord_process_reentry = false;
}


static const char * ft_logrecord_expand_sd(struct ft_logrecord * le)
{
	int rc;
	static char * ft_logrecord_expand_sdbuf = NULL;
	static size_t ft_logrecord_expand_sdbuf_size = 0;

	if (le->sd == NULL) return "";

	size_t len = 3;
	for (const struct ft_log_sd * sd = le->sd; sd->name != NULL; sd += 1)
	{
		len += strlen(sd->name) + 2 + strlen(sd->value) + 2;
	}

	if (ft_logrecord_expand_sdbuf_size < len)
	{
		void * old = ft_logrecord_expand_sdbuf;
		ft_logrecord_expand_sdbuf = realloc(ft_logrecord_expand_sdbuf, len);
		if (ft_logrecord_expand_sdbuf == NULL)
		{
			ft_logrecord_expand_sdbuf = old;
			return "[NOMEM=1] "; // Out-of-memory situation
		}
		ft_logrecord_expand_sdbuf_size = len;
	}

	char * c = ft_logrecord_expand_sdbuf;
	assert(c != NULL);
	assert(c < (ft_logrecord_expand_sdbuf + ft_logrecord_expand_sdbuf_size));
	c[0] = '[';

	c += 1;
	assert(c < (ft_logrecord_expand_sdbuf + ft_logrecord_expand_sdbuf_size));

	for (const struct ft_log_sd * sd = le->sd; sd->name != NULL; sd += 1)
	{
		if (strchr(sd->value, ' ') == NULL)
		{
			// No need to escape the value
			rc = sprintf(c, "%s=%s ", sd->name, sd->value);
			c += rc;
		}
		else
		{
			rc = sprintf(c, "%s=\"", sd->name);
			c += rc;
			for (const char * s = sd->value;  *s != '\0'; s += 1)
			{
				if (*s != '"')
				{
					*c = *s;
					c += 1;
				}
				else
				{
					*c = '`';
					c += 1;
				}
			}
			rc = sprintf(c, "\" ");
			c += rc;
		}
		assert(c < (ft_logrecord_expand_sdbuf + ft_logrecord_expand_sdbuf_size));
	}

	c -= 1;
	assert(c < (ft_logrecord_expand_sdbuf + ft_logrecord_expand_sdbuf_size));
	c[0] = ']';

	c += 1;
	assert(c < (ft_logrecord_expand_sdbuf + ft_logrecord_expand_sdbuf_size));
	c[0] = ' ';

	c += 1;
	assert(c < (ft_logrecord_expand_sdbuf + ft_logrecord_expand_sdbuf_size));
	c[1] = '\0';

	return ft_logrecord_expand_sdbuf;
}

void ft_logrecord_emergency_fprint(struct ft_logrecord * le, int le_message_length, FILE * f)
{
	time_t t = le->timestamp;
	struct tm tmp;
	gmtime_r(&t, &tmp);
	unsigned int frac100 = ((uint64_t)(le->timestamp * 1000.0)) % 1000;

	fprintf(f, 
		"%04d-%02d-%02dT%02d:%02d:%02d.%03uZ %s[%5d]! %s: %s%.*s\n",
		1900+tmp.tm_year, 1+tmp.tm_mon, tmp.tm_mday,
		tmp.tm_hour, tmp.tm_min, tmp.tm_sec, frac100,
		le->appname, le->pid,
		ft_log_levelname(le->level),
		ft_logrecord_expand_sd(le),
		le_message_length, le->message
	);
}


const char * ft_log_months[12] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

const char * ft_log_levelname(char level)
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

