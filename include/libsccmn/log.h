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
	FT_TRACE_ID_SOCK_STREAM = 0x0001,
	FT_TRACE_ID_EVENT_LOOP = 0x0002,
	FT_TRACE_ID_LISTENER = 0x0004,
};


void _ft_log_v(const char level, const char * format, va_list args);
static inline void _ft_log(const char level, const char * format, ...) __attribute__ ((__format__ (__printf__, 2, 3)));
static inline void _ft_log(const char level, const char * format, ...)
{
	if ((level == 'D') && (!libsccmn_config.log_verbose)) return;

	va_list args;
	va_start(args, format);
	_ft_log_v(level, format, args);
	va_end(args);
}

void _ft_log_errno_v(int errnum, const char level, const char * format, va_list args);
static inline void _ft_log_errno(int errnum, const char level, const char *format, ...) __attribute__((format(printf,3,4)));
static inline void _ft_log_errno(int errnum, const char level, const char *format, ...)
{
	if ((level == 'D') && (!libsccmn_config.log_verbose)) return;

	va_list args;
	va_start(args, format);
	_ft_log_errno_v(errnum, level, format, args);
	va_end(args);
}


void _ft_log_openssl_err_v(char const level, const char * format, va_list args);
static inline void _ft_log_openssl_err(char const level, const char * format, ...) __attribute__ ((__format__ (__printf__, 2, 3)));
static inline void _ft_log_openssl_err(char const level, const char * format, ...)
{
	if ((level == 'D') && (!libsccmn_config.log_verbose)) return;

	va_list args;
	va_start(args, format);
	_ft_log_openssl_err_v(level, format, args);
	va_end(args);
}

#ifdef RELEASE
#define FT_TRACE(traceid, fmt, args...) do { if (0) _ft_log('T', "(%04X) %s:%s:%d " fmt, traceid, __FILE__, __func__, __LINE__, ## args); } while (0)
#else
#define FT_TRACE(traceid, fmt, args...) if ((libsccmn_config.log_trace_mask & traceid) != 0) _ft_log('T', "(%04X) %s:%s:%d " fmt, traceid, __FILE__, __func__, __LINE__, ## args)
#endif

#ifdef RELEASE
#define FT_DEBUG(fmt, args...) do { if (0) _ft_log('D', fmt, ## args); } while (0)
#else
#define FT_DEBUG(fmt, args...) _ft_log('D', fmt, ## args)
#endif
#define FT_INFO(fmt, args...)  _ft_log('I', fmt, ## args)
#define FT_WARN(fmt, args...)  _ft_log('W', fmt, ## args)
#define FT_ERROR(fmt, args...) _ft_log('E', fmt, ## args)
#define FT_FATAL(fmt, args...) _ft_log('F', fmt, ## args)
#define FT_AUDIT(fmt, args...) _ft_log('A', fmt, ## args)

#ifdef RELEASE
#define FT_DEBUG_P(fmt, args...) do { if (0) _ft_log('D', "%s:%s:%d " fmt, __FILE__, __func__, __LINE__, ## args); } while (0)
#else
#define FT_DEBUG_P(fmt, args...) _ft_log('D', "%s:%s:%d " fmt, __FILE__, __func__, __LINE__, ## args)
#endif
#define FT_INFO_P(fmt, args...)  _ft_log('I', "%s:%s:%d " fmt, __FILE__, __func__, __LINE__, ## args)
#define FT_WARN_P(fmt, args...)  _ft_log('W', "%s:%s:%d " fmt, __FILE__, __func__, __LINE__, ## args)
#define FT_ERROR_P(fmt, args...) _ft_log('E', "%s:%s:%d " fmt, __FILE__, __func__, __LINE__, ## args)
#define FT_FATAL_P(fmt, args...) _ft_log('F', "%s:%s:%d " fmt, __FILE__, __func__, __LINE__, ## args)
#define FT_AUDIT_P(fmt, args...) _ft_log('A', "%s:%s:%d " fmt, __FILE__, __func__, __LINE__, ## args)

#ifdef RELEASE
#define FT_DEBUG_ERRNO(errnum, fmt, args...) do { if (0) _ft_log_errno(errnum, 'D', fmt, ## args); } while (0)
#else
#define FT_DEBUG_ERRNO(errnum, fmt, args...) _ft_log_errno(errnum, 'D', fmt, ## args)
#endif
#define FT_INFO_ERRNO(errnum, fmt, args...)  _ft_log_errno(errnum, 'I', fmt, ## args)
#define FT_WARN_ERRNO(errnum, fmt, args...)  _ft_log_errno(errnum, 'W', fmt, ## args)
#define FT_ERROR_ERRNO(errnum, fmt, args...) _ft_log_errno(errnum, 'E', fmt, ## args)
#define FT_FATAL_ERRNO(errnum, fmt, args...) _ft_log_errno(errnum, 'F', fmt, ## args)
#define FT_AUDIT_ERRNO(errnum, fmt, args...) _ft_log_errno(errnum, 'A', fmt, ## args)

#ifdef RELEASE
#define FT_DEBUG_ERRNO_P(errnum, fmt, args...) do { if (0) _ft_log_errno(errnum, 'D', "%s:%s:%d " fmt, __FILE__, __func__, __LINE__, ## args); } while (0)
#else
#define FT_INFO_ERRNO_P(errnum, fmt, args...)  _ft_log_errno(errnum, 'I', "%s:%s:%d " fmt, __FILE__, __func__, __LINE__, ## args)
#endif
#define FT_WARN_ERRNO_P(errnum, fmt, args...)  _ft_log_errno(errnum, 'W', "%s:%s:%d " fmt, __FILE__, __func__, __LINE__, ## args)
#define FT_ERROR_ERRNO_P(errnum, fmt, args...) _ft_log_errno(errnum, 'E', "%s:%s:%d " fmt, __FILE__, __func__, __LINE__, ## args)
#define FT_FATAL_ERRNO_P(errnum, fmt, args...) _ft_log_errno(errnum, 'F', "%s:%s:%d " fmt, __FILE__, __func__, __LINE__, ## args)
#define FT_AUDIT_ERRNO_P(errnum, fmt, args...) _ft_log_errno(errnum, 'A', "%s:%s:%d " fmt, __FILE__, __func__, __LINE__, ## args)

#ifdef RELEASE
#define FT_INFO_OPENSSL(fmt, args...) do { if (0) _ft_log_openssl_err('I', fmt, ## args); } while (0)
#else
#define FT_INFO_OPENSSL(fmt, args...)  _ft_log_openssl_err('I', fmt, ## args)
#endif
#define FT_WARN_OPENSSL(fmt, args...)  _ft_log_openssl_err('W', fmt, ## args)
#define FT_ERROR_OPENSSL(fmt, args...) _ft_log_openssl_err('E', fmt, ## args)
#define FT_FATAL_OPENSSL(fmt, args...) _ft_log_openssl_err('F', fmt, ## args)
#define FT_AUDIT_OPENSSL(fmt, args...) _ft_log_openssl_err('A', fmt, ## args)

#ifdef RELEASE
#define FT_INFO_OPENSSL_P(fmt, args...) do { if (0) _ft_log_openssl_err('I', "%s:%s:%d " fmt, __FILE__, __func__, __LINE__, ## args); } while (0)
#else
#define FT_INFO_OPENSSL_P(fmt, args...)  _ft_log_openssl_err('I', "%s:%s:%d " fmt, __FILE__, __func__, __LINE__, ## args)
#endif
#define FT_WARN_OPENSSL_P(fmt, args...)  _ft_log_openssl_err('W', "%s:%s:%d " fmt, __FILE__, __func__, __LINE__, ## args)
#define FT_ERROR_OPENSSL_P(fmt, args...) _ft_log_openssl_err('E', "%s:%s:%d " fmt, __FILE__, __func__, __LINE__, ## args)
#define FT_FATAL_OPENSSL_P(fmt, args...) _ft_log_openssl_err('F', "%s:%s:%d " fmt, __FILE__, __func__, __LINE__, ## args)
#define FT_AUDIT_OPENSSL_P(fmt, args...) _ft_log_openssl_err('A', "%s:%s:%d " fmt, __FILE__, __func__, __LINE__, ## args)

/*
 * Call this to open or reopen log file.
 * Used during initial switch from stderr a log file and eventually to rotate a log file
 */
bool ft_log_reopen(void);
void ft_log_finalise(void);
void ft_log_flush(void);

void ft_log_context(struct context * context);

/*
 * Specify location of the log file.
 * Implicitly manages ft_log_reopen().
 * Can be called also with fname set to NULL to remove filename and switch back to stderr 
 */
bool ft_log_filename(const char * fname);

static inline bool ft_log_is_verbose(void)
{
	return libsccmn_config.log_verbose;
}

static inline void ft_log_verbose(bool v)
{
	libsccmn_config.log_verbose = v;
}

///

struct ft_logrecord
{
	double timestamp;
	pid_t pid;
	char level;
	char message[4096];
};

void ft_logrecord_process(struct ft_logrecord * le, int le_message_length);

///

#endif // FT_LOG_H_
