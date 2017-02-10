#include "_ft_internal.h"

///

//TODO: Remove 'internal' or old log format ("%s %02d %04d %02d:%02d:%02d.%03d %s %6d %s: %.*s\n") below
static void ft_log_file_backend_fini(void);

///

// ft_log_file is a kind of special backend because it works even without init INTENTIONALLY!!
bool ft_log_file_backend_init(const char * fname)
{
	ft_log_file_backend_fini(); // Just for sure

	ft_config.log_file.filename = (fname != NULL) ? strdup(fname) : NULL;
	bool ok = ft_log_file_reopen();
	if (!ok) return false;

	return true;
}


static void ft_log_file_backend_fini()
{
	if (ft_config.log_file.file != NULL)
	{
		fclose(ft_config.log_file.file);
		ft_config.log_file.file = NULL;
	}

	if (ft_config.log_file.filename != NULL)
	{
		free((void *)ft_config.log_file.filename);
		ft_config.log_file.filename = NULL;
	}
}

static const char * ft_log_file_expand_sd(struct ft_logrecord * le)
{
	int rc;
	static __thread char * ft_log_file_expand_sdbuf = NULL;
	static __thread size_t ft_log_file_expand_sdbuf_size = 0;

	if (le->sd == NULL) return "";

	size_t len = 3;
	for (const struct ft_log_sd * sd = le->sd; sd->name != NULL; sd += 1)
	{
		len += strlen(sd->name) + 1 + strlen(sd->value) + 1;
	}

	if (ft_log_file_expand_sdbuf_size < len)
	{
		size_t alloc_len = len;
		if (alloc_len < 256) alloc_len = 256;
		void * old = ft_log_file_expand_sdbuf;
		ft_log_file_expand_sdbuf = realloc(ft_log_file_expand_sdbuf, alloc_len);
		if (ft_log_file_expand_sdbuf == NULL)
		{
			ft_log_file_expand_sdbuf = old;
			return "[NOMEM=1] "; // Out-of-memory situation
		}
		ft_log_file_expand_sdbuf_size = alloc_len;
	}

	char * c = ft_log_file_expand_sdbuf;

	c[0] = '[';
	c += 1;

	for (const struct ft_log_sd * sd = le->sd; sd->name != NULL; sd += 1)
	{
		rc = sprintf(c, "%s=%s ", sd->name, sd->value);
		c += rc;
	}

	c[-1] = ']';
	c[0] = ' ';
	c[1] = '\0';

	return ft_log_file_expand_sdbuf;
}


static void ft_log_file_backend_logrecord_process(struct ft_logrecord * le, int le_message_length)
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
			"%04d-%02d-%02dT%02d:%02d:%02d.%03dZ %s[%5d] %s: %s%.*s\n",
			1900+tmp.tm_year, 1+tmp.tm_mon, tmp.tm_mday,
			tmp.tm_hour, tmp.tm_min, tmp.tm_sec, frac100,
			le->appname, le->pid,
			ft_log_levelname(le->level),
			ft_log_file_expand_sd(le),
			le_message_length, le->message
		);
	} else {
		fprintf(ft_config.log_file.file != NULL ? ft_config.log_file.file : stderr, 
			"%s %02d %04d %02d:%02d:%02d.%03d %s %s[%5d] %s: %s%.*s\n",
			ft_log_months[tmp.tm_mon], tmp.tm_mday, 1900+tmp.tm_year,
			tmp.tm_hour, tmp.tm_min, tmp.tm_sec, frac100,
			tmp.tm_zone,
			le->appname, le->pid,
			ft_log_levelname(le->level),
			ft_log_file_expand_sd(le),
			le_message_length, le->message
		);
	}

	ft_config.log_file.flush_counter += 1;
}

static void ft_log_file_flush(ev_tstamp now)
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

static void ft_log_file_backend_on_prepare(struct ft_context * context, ev_tstamp now)
{
	ft_log_file_flush(now);
}

static void ft_log_file_on_sighup()
{
	ft_log_file_reopen();
}


struct ft_log_backend ft_log_file_backend = {
	.fini = ft_log_file_backend_fini,
	.process = ft_log_file_backend_logrecord_process,
	.flush = ft_log_file_flush,
	.on_prepare = ft_log_file_backend_on_prepare,
	.on_sighup = ft_log_file_on_sighup
};


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
		fclose(ft_config.log_file.file);
		ft_config.log_file.file = NULL;
	}

	ft_config.log_file.file = f;

	if (reopen)
	{
		FT_INFO("Log file is reopen");
	}

	return true;
}

