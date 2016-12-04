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

static const char * ft_log_syslog_months[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

// Forward declarations
static struct ft_dgram_delegate ft_log_syslog_backend_dgram_delegate;
static bool ft_log_syslog_connect(void);
static void ft_log_syslog_send(bool alloc_new_frame);
static bool ft_log_syslog_frame_alloc(void);

///

bool ft_log_syslog_backend_init(struct ft_context * context)
{
	bool ok;

	if (ft_log_syslog_dgram.base.socket.clazz != NULL)
	{
		FT_WARN("ft_log_syslog_backend aalready initialized");
		return false;
	}

	ok = ft_dgram_init(&ft_log_syslog_dgram, &ft_log_syslog_backend_dgram_delegate, context, AF_LOCAL, SOCK_DGRAM, 0);
	if (!ok) return false;

	ok = ft_log_syslog_connect();
	if (!ok) return false;

	if (ft_log_syslog_frame == NULL)
	{
		ok = ft_log_syslog_frame_alloc();
		if (!ok) return false;
	}

	gethostname(ft_config.log_syslog.hostname, sizeof(ft_config.log_syslog.hostname)-1);
	char * c = strchr(ft_config.log_syslog.hostname, '.');
	if (c != NULL) *c = '\0';

	return true;
}

static void ft_log_syslog_backend_fini()
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
			ft_frame_return(ft_log_syslog_frame);
			ft_log_syslog_frame = NULL;
		}
	}


	if (ft_log_syslog_dgram.base.socket.clazz != NULL)
	{
		ft_dgram_fini(&ft_log_syslog_dgram);
	}

}

static void ft_log_syslog_backend_logrecord_process(struct ft_logrecord * le, int le_message_length)
{
	bool ok;
	static bool ft_log_syslog_backend_logrecord_process_reentry = false;
	if (ft_log_syslog_backend_logrecord_process_reentry == true)
	{
		//TODO: Print log message to stdout
		fprintf(stderr, "Reentry of ft_log_syslog_backend_logrecord_process, that's not supported\n");
		return;
	}

retry:
	ft_log_syslog_backend_logrecord_process_reentry = true;

	// !!!! Try to use functions that don't log anything, this is not reentrant !!!
	assert(ft_log_syslog_frame != NULL);

	struct ft_vec * vec = ft_frame_append_vec(ft_log_syslog_frame, le_message_length + 200);
	if (vec == NULL)
	{
		ft_log_syslog_backend_logrecord_process_reentry = false;
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
		case 'I': pri |= 6; level = "INFO"; break;
		case 'D': pri |= 7; level = "DEBUG"; break;
		case 'T': pri |= 7; level = "TRACE"; break;
		default:  pri |= 7; level = "?"; break;
	}

	time_t t = le->timestamp;
	struct tm tmp;
	unsigned int frac100 = (le->timestamp * 1000) - (t * 1000);
	gmtime_r(&t, &tmp);

	// Best-effort string, here is an example:
	// <133>Dec 03 06:30:00 myhost seacat-gw 12345 DEBUG123: [ft@1 t=2016-12-03T06:30:00.123Z] A free-form message that provides information about the event
	ok = ft_vec_sprintf(vec, "<%d>%s %2d %02d:%02d:%02d %s %s[%d]: [t=%04d-%02d-%02dT%02d:%02d:%02d.%03dZ l=%s] %s\n",
		pri,
		ft_log_syslog_months[tmp.tm_mon], tmp.tm_mday,
		tmp.tm_hour, tmp.tm_min, tmp.tm_sec,
		ft_config.log_syslog.hostname,
		"seacat-gw",
		le->pid,
		1900+tmp.tm_year, 1+tmp.tm_mon, tmp.tm_mday,
		tmp.tm_hour, tmp.tm_min, tmp.tm_sec, frac100,
		level, //TODO: Extend with optional message id (e.g. INFO354)
		le->message
	);
	if (!ok)
	{
		fprintf(stderr, "ft_log_syslog_backend_logrecord_process ft_vec_sprintf failed.\n");
		return;
	}

	ft_vec_cutoff(vec);

	ft_log_syslog_backend_logrecord_process_reentry = false;
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

///

struct ft_log_backend ft_log_syslog_backend = {
	.fini = ft_log_syslog_backend_fini,
	.process = ft_log_syslog_backend_logrecord_process,
	.on_prepare = ft_log_syslog_backend_on_prepare,
};

static struct ft_dgram_delegate ft_log_syslog_backend_dgram_delegate = {
	.read = NULL, //TODO: This ...
	.error = ft_log_syslog_on_dgram_error,
};
