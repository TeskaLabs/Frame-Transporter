#include "_ft_internal.h"

/*
 * The BSD syslog Protocol https://tools.ietf.org/html/rfc3164
 * The Syslog Protocol https://tools.ietf.org/html/rfc5424
 */

///

static struct ft_dgram ft_log_syslog_dgram = {
	.base.socket.clazz = NULL,
};

static struct ft_frame * ft_log_syslog_frame = NULL;

// Forward declarations
static struct ft_dgram_delegate ft_log_syslog_backend_dgram_delegate;
static bool ft_log_syslog_connect(void);
static void ft_log_syslog_send(bool alloc_new_frame);
static bool ft_log_syslog_frame_alloc(void);
static void ft_log_syslog_backend_flush(ev_tstamp now);

///

bool ft_log_syslog_backend_init(struct ft_context * context)
{
	bool ok;

	if (ft_log_syslog_dgram.base.socket.clazz != NULL)
	{
		FT_WARN("ft_log_syslog_backend already initialized");
		return false;
	}

	ok = ft_dgram_init(&ft_log_syslog_dgram, &ft_log_syslog_backend_dgram_delegate, context, AF_LOCAL, SOCK_DGRAM, 0);
	if (!ok) return false;

	ok = ft_log_syslog_connect();
	if (!ok) return false;

	if (ft_config.log_syslog.hostname == NULL)
	{
		char hostname[1024];
		gethostname(hostname, sizeof(hostname));
		char * c = strchr(hostname, '.');
		if (c != NULL) *c ='\0';
		ft_config.log_syslog.hostname = strdup(hostname);
	}

	if (ft_config.log_syslog.domainname == NULL)
	{
		char hostname[1024];
		gethostname(hostname, sizeof(hostname));
		ft_config.log_syslog.domainname = strdup(hostname);
	}

	return true;
}

static void ft_log_syslog_backend_fini()
{
	ft_log_syslog_backend_flush(1e77); // Flush at any cost

	if (ft_log_syslog_dgram.base.socket.clazz != NULL)
	{
		ft_dgram_fini(&ft_log_syslog_dgram);
	}
}


static const char * ft_log_syslog_backend_expand_sd(struct ft_logrecord * le, char separator)
{
	int rc;
	static __thread char * ft_log_syslog_backend_expand_sdbuf = NULL;
	static __thread size_t ft_log_syslog_backend_expand_sdbuf_size = 0;

	if (le->sd == NULL) return "";

	size_t len = 2;
	for (const struct ft_log_sd * sd = le->sd; sd->name != NULL; sd += 1)
	{
		len += strlen(sd->name) + 4 + strlen(sd->value);
	}

	if (ft_log_syslog_backend_expand_sdbuf_size < len)
	{
		void * old = ft_log_syslog_backend_expand_sdbuf;
		ft_log_syslog_backend_expand_sdbuf = realloc(ft_log_syslog_backend_expand_sdbuf, len);
		if (ft_log_syslog_backend_expand_sdbuf == NULL)
		{
			ft_log_syslog_backend_expand_sdbuf = old;
			return " NOMEM=1"; // Out-of-memory situation
		}
		ft_log_syslog_backend_expand_sdbuf_size = len;
	}

	char * c = ft_log_syslog_backend_expand_sdbuf;

	c[0] = separator;
	c += 1;

	for (const struct ft_log_sd * sd = le->sd; sd->name != NULL; sd += 1)
	{
		rc = sprintf(c, "%s=\"%s\"%c", sd->name, sd->value, separator);
		c += rc;
		assert(c < (ft_log_syslog_backend_expand_sdbuf + ft_log_syslog_backend_expand_sdbuf_size));
	}
	c[-1] = '\0';

	return ft_log_syslog_backend_expand_sdbuf;
}


static void ft_log_syslog_backend_logrecord_process(struct ft_logrecord * le, int le_message_length)
{
	bool ok;

retry:
	if (ft_log_syslog_frame == NULL)
	{
		ok = ft_log_syslog_frame_alloc();
		if (!ok)
		{
			//TODO: Swap this for 'emergency log function'
			ft_logrecord_fprint(le, le_message_length, stderr);

			FT_WARN("No memory for syslog frame");
			return;
		}
	}

	struct ft_vec * vec = ft_frame_append_vec(ft_log_syslog_frame, le_message_length + 200);
	if (vec == NULL)
	{
		ft_log_syslog_send(true);
		goto retry;
	}

	const char * level;
	int pri = ft_config.log_syslog.facility << 3;
	switch (le->level)
	{
		case 'F': pri |= 2; level = "FATAL"; break;
		case 'E': pri |= 3; level = "ERROR"; break;
		case 'W': pri |= 4; level = "WARN"; break;
		case 'A': pri |= 5; level = "AUDIT"; break;
		case 'I': pri |= 5; level = "INFO"; break;
		case 'D': pri |= 7; level = "DEBUG"; break;
		case 'T': pri |= 7; level = "TRACE"; break;
		default:  pri |= 7; level = "?"; break;
	}

	time_t t = le->timestamp;
	struct tm tmp;
	unsigned int frac100 = (le->timestamp * 1000) - (t * 1000);

	switch (ft_config.log_syslog.format)
	{
		case 'm':
			localtime_r(&t, &tmp);
			ok = ft_vec_sprintf(vec, "<%d>%s %2d %02d:%02d:%02d %s[%d]: %s:%s%s\n",
				pri,
				ft_log_months[tmp.tm_mon], tmp.tm_mday,
				tmp.tm_hour, tmp.tm_min, tmp.tm_sec,
				le->appname,
				le->pid,
				level, //TODO: Extend with optional message id (e.g. INFO354)
				le->message,
				ft_log_syslog_backend_expand_sd(le, ';')
			);
			break;

		case '3':
			// RFC3164
			localtime_r(&t, &tmp);
			ok = ft_vec_sprintf(vec, "<%d>%s %d %02d:%02d:%02d %s %s[%d]: [t=\"%04d-%02d-%02dT%02d:%02d:%02d.%03dZ\"%s] %s: %s\n",
				pri,
				ft_log_months[tmp.tm_mon], tmp.tm_mday,
				tmp.tm_hour, tmp.tm_min, tmp.tm_sec,
				ft_config.log_syslog.hostname,
				le->appname,
				le->pid,
				1900+tmp.tm_year, 1+tmp.tm_mon, tmp.tm_mday,
				tmp.tm_hour, tmp.tm_min, tmp.tm_sec, frac100,
				ft_log_syslog_backend_expand_sd(le, ' '),
				level, //TODO: Extend with optional message id (e.g. INFO354)
				le->message
			);
			break;

		case '5':
		default:
			// RFC5424
			gmtime_r(&t, &tmp);
			// l@47278 is from http://oidref.com/1.3.6.1.4.1.47278 -> TeskaLabs 'SMI Network Management Private Enterprise Code', maintained by IANA,
			// whose prefix is iso.org.dod.internet.private.enterprise (1.3.6.1.4.1)
			ok = ft_vec_sprintf(vec, "<%d>1 %04d-%02d-%02dT%02d:%02d:%02d.%03dZ %s %s %d - [l@47278 l=\"%s\"%s] %s\n",
				pri,
				1900+tmp.tm_year, 1+tmp.tm_mon, tmp.tm_mday,
				tmp.tm_hour, tmp.tm_min, tmp.tm_sec, frac100,
				ft_config.log_syslog.domainname,
				le->appname,
				le->pid,
				level,
				ft_log_syslog_backend_expand_sd(le, ' '),
				le->message
			);
			break;
	}

	if (!ok)
	{
		FT_WARN("Failed to dump log entry into syslog format");
		//TODO: Swap this for 'emergency log function'
		ft_logrecord_fprint(le, le_message_length, stderr);
		return;
	}

	ft_vec_trim(vec);
}

static void ft_log_syslog_send(bool alloc_new_frame)
{
	bool ok;

	struct ft_frame * frame = ft_log_syslog_frame;
	ft_log_syslog_frame = NULL;
	if (alloc_new_frame) ft_log_syslog_frame_alloc();

	if (frame != NULL)
	{
		ft_frame_flip(frame);

		//This line dumps syslog 'raw' format to stderr
		//ft_frame_fwrite(frame, stderr);

		ok = ft_dgram_write(&ft_log_syslog_dgram, frame);
		if (!ok)
		{
			//TODO: Dump log content into a new frame
			ft_frame_return(frame);
		}
	}
}


static void ft_log_syslog_backend_on_prepare(struct ft_context * context, ev_tstamp now)
{
	// Make sure that we have a frame
	if (ft_log_syslog_frame == NULL)
	{
		ft_log_syslog_frame_alloc();
	}

	//TODO: There can be configurable limit for log submit
	if (ft_log_syslog_frame->vec_limit <= 0) return;

	ft_log_syslog_send(true);
}

static bool ft_log_syslog_connect()
{
	bool ok;

	struct sockaddr_un addr;
	addr.sun_family = AF_LOCAL;
	strncpy(addr.sun_path, ft_config.log_syslog.address, sizeof (addr.sun_path));
	addr.sun_path[sizeof(addr.sun_path) - 1] = '\0';

	ok = ft_dgram_connect(&ft_log_syslog_dgram, (const struct sockaddr *)&addr, sizeof(addr));
	if (!ok)
	{
		ft_dgram_fini(&ft_log_syslog_dgram);
		return false;
	}

	return true;
}

static bool ft_log_syslog_frame_alloc()
{
	assert(ft_log_syslog_frame == NULL);
	ft_log_syslog_frame = ft_pool_borrow(&ft_log_syslog_dgram.base.socket.context->frame_pool, FT_FRAME_TYPE_LOG);
	if (ft_log_syslog_frame == NULL)
	{
		fprintf(stderr, "Out-of-frame situation\n");
		return false;
	}

	ft_frame_format_empty(ft_log_syslog_frame);
	return true;
}

static void ft_log_syslog_on_dgram_error(struct ft_dgram * dgram)
{
	fprintf(stderr, "ERROR: %d\n", dgram->error.sys_errno);
	//TODO: This - probably try to reconnect
}


static void ft_log_syslog_backend_flush(ev_tstamp now)
{
	if (ft_log_syslog_frame != NULL)
	{
		if (ft_log_syslog_frame->vec_limit > 0)
		{
			ft_log_syslog_send(false);
		}
		
		if (ft_log_syslog_frame != NULL)
		{
			if (ft_log_syslog_frame->vec_limit > 0) FT_WARN("Lost a data in a syslog log frame");
			struct ft_frame * frame = ft_log_syslog_frame;
			ft_log_syslog_frame = NULL;
			ft_frame_return(frame);
			
		}
	}

	if (ft_log_syslog_dgram.base.socket.clazz != NULL)
	{
		ft_dgram_flush(&ft_log_syslog_dgram);
	}
}

///

struct ft_log_backend ft_log_syslog_backend = {
	.fini = ft_log_syslog_backend_fini,
	.process = ft_log_syslog_backend_logrecord_process,
	.flush = ft_log_syslog_backend_flush,
	.on_prepare = ft_log_syslog_backend_on_prepare,
	.on_sighup = NULL,
};

static struct ft_dgram_delegate ft_log_syslog_backend_dgram_delegate = {
	.read = NULL, //TODO: This ...
	.error = ft_log_syslog_on_dgram_error,
};
