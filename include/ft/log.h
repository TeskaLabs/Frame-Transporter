#ifndef FT_LOG_H_
#define FT_LOG_H_

/*
Log levels:
  'T': trace
  'D': debug
  'I': info
  'W': warning
  'E': error
  'F': fatal
  'A': audit
*/

enum ft_log_trace_ids
{
	FT_TRACE_ID_EVENT_LOOP = 0x0002,
	FT_TRACE_ID_LISTENER = 0x0004,
	FT_TRACE_ID_MEMPOOL = 0x0008,

	FT_TRACE_ID_STREAM = 0x00010,
	FT_TRACE_ID_DGRAM = 0x0020,

	FT_TRACE_ID_PROTO_SOCK = 0x0100,
};


struct ft_log_stats
{
	unsigned int trace_count;
	unsigned int debug_count;
	unsigned int info_count;
	unsigned int warn_count;
	unsigned int error_count;
	unsigned int fatal_count;
	unsigned int audit_count;
};

extern struct ft_log_stats ft_log_stats;


// Structured data
struct ft_log_sd
{
	const char * name;
	const char * value;
};


void _ft_log_v(const char level, const struct ft_log_sd sd[], const char * format, va_list args);
static inline void _ft_log(const char level, const struct ft_log_sd sd[], const char * format, ...) __attribute__ ((__format__ (__printf__, 3, 4)));
static inline void _ft_log(const char level, const struct ft_log_sd sd[], const char * format, ...)
{
	va_list args;
	va_start(args, format);
	_ft_log_v(level, sd, format, args);
	va_end(args);
}

void _ft_log_errno_v(int errnum, const char level, const struct ft_log_sd sd[], const char * format, va_list args);
static inline void _ft_log_errno(int errnum, const char level, const char *format, ...) __attribute__((format(printf,3,4)));
static inline void _ft_log_errno(int errnum, const char level, const char *format, ...)
{
	va_list args;
	va_start(args, format);
	_ft_log_errno_v(errnum, level, NULL, format, args);
	va_end(args);
}


void _ft_log_openssl_err_v(char const level, const struct ft_log_sd sd[], const char * format, va_list args);
static inline void _ft_log_openssl_err(char const level, const char * format, ...) __attribute__ ((__format__ (__printf__, 2, 3)));
static inline void _ft_log_openssl_err(char const level, const char * format, ...)
{
	va_list args;
	va_start(args, format);
	_ft_log_openssl_err_v(level, NULL, format, args);
	va_end(args);
}

#ifdef RELEASE
#define FT_TRACE(traceid, fmt, args...) do { if (0) _ft_log('T', NULL, "%04X %s:%s:%d " fmt, traceid, __FILE__, __func__, __LINE__, ## args); } while (0)
#else
#define FT_TRACE(traceid, fmt, args...) do { if ((ft_config.log_trace_mask & traceid) == 0) break; _ft_log('T', NULL, "%04X %s:%s:%d " fmt, traceid, __FILE__, __func__, __LINE__, ## args); } while (0)
#endif

#ifdef RELEASE
#define FT_DEBUG(fmt, args...) do { if (0) _ft_log('D', NULL, fmt, ## args); } while (0)
#else
#define FT_DEBUG(fmt, args...) do { if (!ft_config.log_verbose) break; ft_log_stats.debug_count += 1; _ft_log('D', NULL, fmt, ## args); } while (0)
#endif
#define FT_INFO(fmt, args...)  do { ft_log_stats.info_count  += 1; _ft_log('I', NULL, fmt, ## args); } while (0)
#define FT_WARN(fmt, args...)  do { ft_log_stats.warn_count  += 1; _ft_log('W', NULL, fmt, ## args); } while (0)
#define FT_ERROR(fmt, args...) do { ft_log_stats.error_count += 1; _ft_log('E', NULL, fmt, ## args); } while (0)
#define FT_FATAL(fmt, args...) do { ft_log_stats.fatal_count += 1; _ft_log('F', NULL, fmt, ## args); } while (0)
#define FT_AUDIT(fmt, args...) do { ft_log_stats.audit_count += 1; _ft_log('A', NULL, fmt, ## args); } while (0)

#define FT_AUDIT_SD(sd, fmt, args...) do { ft_log_stats.audit_count += 1; _ft_log('A', sd, fmt, ## args); } while (0)

#ifdef RELEASE
#define FT_DEBUG_P(fmt, args...) do { if (0) _ft_log('D', NULL, "%s:%s:%d " fmt, __FILE__, __func__, __LINE__, ## args); } while (0)
#else
#define FT_DEBUG_P(fmt, args...) do { if (!ft_config.log_verbose) break; ft_log_stats.debug_count += 1;  _ft_log('D', NULL, "%s:%s:%d " fmt, __FILE__, __func__, __LINE__, ## args); } while (0)
#endif
#define FT_INFO_P(fmt, args...)  do { ft_log_stats.info_count  += 1; _ft_log('I', NULL, "%s:%s:%d " fmt, __FILE__, __func__, __LINE__, ## args); } while (0)
#define FT_WARN_P(fmt, args...)  do { ft_log_stats.warn_count  += 1; _ft_log('W', NULL, "%s:%s:%d " fmt, __FILE__, __func__, __LINE__, ## args); } while (0)
#define FT_ERROR_P(fmt, args...) do { ft_log_stats.error_count += 1; _ft_log('E', NULL, "%s:%s:%d " fmt, __FILE__, __func__, __LINE__, ## args); } while (0)
#define FT_FATAL_P(fmt, args...) do { ft_log_stats.fatal_count += 1; _ft_log('F', NULL, "%s:%s:%d " fmt, __FILE__, __func__, __LINE__, ## args); } while (0)
#define FT_AUDIT_P(fmt, args...) do { ft_log_stats.audit_count += 1; _ft_log('A', NULL, "%s:%s:%d " fmt, __FILE__, __func__, __LINE__, ## args); } while (0)

#ifdef RELEASE
#define FT_DEBUG_ERRNO(errnum, fmt, args...) do { if (0) _ft_log_errno(errnum, 'D', fmt, ## args); } while (0)
#else
#define FT_DEBUG_ERRNO(errnum, fmt, args...) do { if (!ft_config.log_verbose) break; ft_log_stats.debug_count += 1; _ft_log_errno(errnum, 'D', fmt, ## args); } while (0)
#endif
#define FT_INFO_ERRNO(errnum, fmt, args...)  do { ft_log_stats.info_count  += 1; _ft_log_errno(errnum, 'I', fmt, ## args); } while (0)
#define FT_WARN_ERRNO(errnum, fmt, args...)  do { ft_log_stats.warn_count  += 1; _ft_log_errno(errnum, 'W', fmt, ## args); } while (0)
#define FT_ERROR_ERRNO(errnum, fmt, args...) do { ft_log_stats.error_count += 1; _ft_log_errno(errnum, 'E', fmt, ## args); } while (0)
#define FT_FATAL_ERRNO(errnum, fmt, args...) do { ft_log_stats.fatal_count += 1; _ft_log_errno(errnum, 'F', fmt, ## args); } while (0)
#define FT_AUDIT_ERRNO(errnum, fmt, args...) do { ft_log_stats.audit_count += 1; _ft_log_errno(errnum, 'A', fmt, ## args); } while (0)

#ifdef RELEASE
#define FT_DEBUG_ERRNO_P(errnum, fmt, args...) do { if (0) _ft_log_errno(errnum, 'D', "%s:%s:%d " fmt, __FILE__, __func__, __LINE__, ## args); } while (0)
#else
#define FT_DEBUG_ERRNO_P(errnum, fmt, args...) do { if (!ft_config.log_verbose) break; ft_log_stats.debug_count += 1; _ft_log_errno(errnum, 'D', "%s:%s:%d " fmt, __FILE__, __func__, __LINE__, ## args); } while (0)
#endif
#define FT_INFO_ERRNO_P(errnum, fmt, args...)  do { ft_log_stats.info_count  += 1; _ft_log_errno(errnum, 'I', "%s:%s:%d " fmt, __FILE__, __func__, __LINE__, ## args); } while (0)
#define FT_WARN_ERRNO_P(errnum, fmt, args...)  do { ft_log_stats.warn_count  += 1; _ft_log_errno(errnum, 'W', "%s:%s:%d " fmt, __FILE__, __func__, __LINE__, ## args); } while (0)
#define FT_ERROR_ERRNO_P(errnum, fmt, args...) do { ft_log_stats.error_count += 1; _ft_log_errno(errnum, 'E', "%s:%s:%d " fmt, __FILE__, __func__, __LINE__, ## args); } while (0)
#define FT_FATAL_ERRNO_P(errnum, fmt, args...) do { ft_log_stats.fatal_count += 1; _ft_log_errno(errnum, 'F', "%s:%s:%d " fmt, __FILE__, __func__, __LINE__, ## args); } while (0)
#define FT_AUDIT_ERRNO_P(errnum, fmt, args...) do { ft_log_stats.audit_count += 1; _ft_log_errno(errnum, 'A', "%s:%s:%d " fmt, __FILE__, __func__, __LINE__, ## args); } while (0)

#ifdef RELEASE
#define FT_DEBUG_OPENSSL(fmt, args...) do { if (0) _ft_log_openssl_err('D', fmt, ## args); } while (0)
#else
#define FT_DEBUG_OPENSSL(fmt, args...) do { if (!ft_config.log_verbose) break; ft_log_stats.debug_count += 1; _ft_log_openssl_err('D', fmt, ## args); } while (0)
#endif
#define FT_INFO_OPENSSL(fmt, args...)  do { ft_log_stats.info_count  += 1; _ft_log_openssl_err('I', fmt, ## args); } while (0)
#define FT_WARN_OPENSSL(fmt, args...)  do { ft_log_stats.warn_count  += 1; _ft_log_openssl_err('W', fmt, ## args); } while (0)
#define FT_ERROR_OPENSSL(fmt, args...) do { ft_log_stats.error_count += 1; _ft_log_openssl_err('E', fmt, ## args); } while (0)
#define FT_FATAL_OPENSSL(fmt, args...) do { ft_log_stats.fatal_count += 1; _ft_log_openssl_err('F', fmt, ## args); } while (0)
#define FT_AUDIT_OPENSSL(fmt, args...) do { ft_log_stats.audit_count += 1; _ft_log_openssl_err('A', fmt, ## args); } while (0)

#ifdef RELEASE
#define FT_DEBUG_OPENSSL_P(fmt, args...) do { if (0) _ft_log_openssl_err('D', "%s:%s:%d " fmt, __FILE__, __func__, __LINE__, ## args); } while (0) }
#else
#define FT_DEBUG_OPENSSL_P(fmt, args...) do { if (!ft_config.log_verbose) break; ft_log_stats.debug_count += 1; _ft_log_openssl_err('D', "%s:%s:%d " fmt, __FILE__, __func__, __LINE__, ## args); } while (0)
#endif
#define FT_INFO_OPENSSL_P(fmt, args...)  do { ft_log_stats.info_count  += 1; _ft_log_openssl_err('I', "%s:%s:%d " fmt, __FILE__, __func__, __LINE__, ## args); } while (0)
#define FT_WARN_OPENSSL_P(fmt, args...)  do { ft_log_stats.warn_count  += 1; _ft_log_openssl_err('W', "%s:%s:%d " fmt, __FILE__, __func__, __LINE__, ## args); } while (0)
#define FT_ERROR_OPENSSL_P(fmt, args...) do { ft_log_stats.error_count += 1; _ft_log_openssl_err('E', "%s:%s:%d " fmt, __FILE__, __func__, __LINE__, ## args); } while (0)
#define FT_FATAL_OPENSSL_P(fmt, args...) do { ft_log_stats.fatal_count += 1; _ft_log_openssl_err('F', "%s:%s:%d " fmt, __FILE__, __func__, __LINE__, ## args); } while (0)
#define FT_AUDIT_OPENSSL_P(fmt, args...) do { ft_log_stats.audit_count += 1; _ft_log_openssl_err('A', "%s:%s:%d " fmt, __FILE__, __func__, __LINE__, ## args); } while (0)

///

void ft_log_context(struct ft_context * context);

static inline bool ft_log_is_verbose(void)
{
	return ft_config.log_verbose;
}

static inline void ft_log_verbose(bool v)
{
	ft_config.log_verbose = v;
}

///

// This structure is designed to be short-lived, within one event loop generation or shorter
// It is not meant to be queued etc. - if needed, serialize the data etc.
struct ft_logrecord
{
	ev_tstamp timestamp;
	pid_t pid;
	char level;
	const char * appname; // https://tools.ietf.org/html/rfc5424#section-6.2.5
	const struct ft_log_sd * sd; // Structured data array terminated by NULL or NULL
	char message[2048]; // Length is somehow defined in https://tools.ietf.org/html/rfc5424#section-6.1
};

void ft_logrecord_process(struct ft_logrecord * le, int le_message_length);

// Print log record to the file, can be used for 'emergency' logging
void ft_logrecord_fprint(struct ft_logrecord * le, int le_message_length, FILE * f);


const char * ft_log_months[12];
const char * ft_log_levelname(char level);

///

struct ft_log_backend
{
	void (*fini)(void);
	void (*process)(struct ft_logrecord * le, int le_message_length);
	void (*on_prepare)(struct ft_context * context, ev_tstamp now);
	void (*on_forkexec)(void); // Called from forked child prior hard discard of the log context, similar to fini() but must be 'harder'
};

// Call this from child after fork to reset logging prior exec
// It is mainly to prevent file descriptors leak
void ft_log_forkexec(void);

void ft_log_finalise(void);

///

void ft_log_backend_switch(struct ft_log_backend * backend);

///

extern struct ft_log_backend ft_log_file_backend;
/*
 * Specify location of the log file.
 * Implicitly manages ft_log_reopen().
 * Can be called also with fname set to NULL to remove filename and switch back to stderr 
 */
bool ft_log_file_backend_init(const char * fname);

///

extern struct ft_log_backend ft_log_syslog_backend;

bool ft_log_syslog_backend_init(struct ft_context * context);
bool ft_log_syslog_set_format(char format); // 3 for RFC3164, 5 for RFC5424, C for compatibility mode

///

// Install log rotate handler on SIGHUP
void ft_log_handle_sighup(struct ft_context * context);

void ft_log_file_flush(ev_tstamp now);

/*
 * Call this to open or reopen log file.
 * Used during initial switch from stderr a log file and eventually to rotate a log file
 */
bool ft_log_file_reopen(void);

#endif // FT_LOG_H_
