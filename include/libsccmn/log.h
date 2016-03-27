#ifndef __LIBSCCMN_LOG_H__
#define __LIBSCCMN_LOG_H__

/*
Log levels:
  'D': debug
  'I': info
  'W': warning
  'E': error
  'F': fatal
  'A': audit
*/

void _log_v(char level, const char * format, va_list args);
static inline void _log(char level, const char * format, ...) __attribute__ ((__format__ (__printf__, 2, 3)));
static inline void _log(char level, const char * format, ...)
{
	if ((level == 'D') && (!libsccmn_config.log_verbose)) return;

	va_list args;
	va_start(args, format);
	_log_v(level, format, args);
	va_end(args);
}

void _log_errno_v(int errnum, char level, const char * format, va_list args);
static inline void _log_errno(int errnum, char level, const char *format, ...) __attribute__((format(printf,3,4)));
static inline void _log_errno(int errnum, char level, const char *format, ...)
{
	if ((level == 'D') && (!libsccmn_config.log_verbose)) return;

	va_list args;
	va_start(args, format);
	_log_errno_v(errnum, level, format, args);
	va_end(args);
}

/*
void _log_openssl_err_v(char level, const char * format, va_list args);
static inline void _log_openssl_err(char level, const char * format, ...) __attribute__ ((__format__ (__printf__, 2, 3)));
static inline void _log_openssl_err(char level, const char * format, ...)
{
	if ((level == 'D') && (!libsccmn_config.log_verbose)) return;

	va_list args;
	va_start(args, format);
	_log_openssl_err_v(level, format, args);
	va_end(args);
}
*/

#define L_DEBUG(fmt, args...) _log('D', fmt, ## args)
#define L_INFO(fmt, args...)  _log('I', fmt, ## args)
#define L_WARN(fmt, args...)  _log('W', fmt, ## args)
#define L_ERROR(fmt, args...) _log('E', fmt, ## args)
#define L_FATAL(fmt, args...) _log('F', fmt, ## args)
#define L_AUDIT(fmt, args...) _log('A', fmt, ## args)

#define L_DEBUG_P(fmt, args...) _log('D', "%s:%s:%d " fmt, __FILE__, __func__, __LINE__, ## args)
#define L_INFO_P(fmt, args...)  _log('I', "%s:%s:%d " fmt, __FILE__, __func__, __LINE__, ## args)
#define L_WARN_P(fmt, args...)  _log('W', "%s:%s:%d " fmt, __FILE__, __func__, __LINE__, ## args)
#define L_ERROR_P(fmt, args...) _log('E', "%s:%s:%d " fmt, __FILE__, __func__, __LINE__, ## args)
#define L_FATAL_P(fmt, args...) _log('F', "%s:%s:%d " fmt, __FILE__, __func__, __LINE__, ## args)
#define L_AUDIT_P(fmt, args...) _log('A', "%s:%s:%d " fmt, __FILE__, __func__, __LINE__, ## args)

#define L_DEBUG_ERRNO(errnum, fmt, args...) _log_errno(errnum, 'D', fmt, ## args)
#define L_INFO_ERRNO(errnum, fmt, args...)  _log_errno(errnum, 'I', fmt, ## args)
#define L_WARN_ERRNO(errnum, fmt, args...)  _log_errno(errnum, 'W', fmt, ## args)
#define L_ERROR_ERRNO(errnum, fmt, args...) _log_errno(errnum, 'E', fmt, ## args)
#define L_FATAL_ERRNO(errnum, fmt, args...) _log_errno(errnum, 'F', fmt, ## args)
#define L_AUDIT_ERRNO(errnum, fmt, args...) _log_errno(errnum, 'A', fmt, ## args)

#define L_DEBUG_ERRNO_P(errnum, fmt, args...) _log_errno(errnum, 'D', "%s:%s:%d " fmt, __FILE__, __func__, __LINE__, ## args)
#define L_INFO_ERRNO_P(errnum, fmt, args...)  _log_errno(errnum, 'I', "%s:%s:%d " fmt, __FILE__, __func__, __LINE__, ## args)
#define L_WARN_ERRNO_P(errnum, fmt, args...)  _log_errno(errnum, 'W', "%s:%s:%d " fmt, __FILE__, __func__, __LINE__, ## args)
#define L_ERROR_ERRNO_P(errnum, fmt, args...) _log_errno(errnum, 'E', "%s:%s:%d " fmt, __FILE__, __func__, __LINE__, ## args)
#define L_FATAL_ERRNO_P(errnum, fmt, args...) _log_errno(errnum, 'F', "%s:%s:%d " fmt, __FILE__, __func__, __LINE__, ## args)
#define L_AUDIT_ERRNO_P(errnum, fmt, args...) _log_errno(errnum, 'A', "%s:%s:%d " fmt, __FILE__, __func__, __LINE__, ## args)

/*
#define L_DEBUG_OPENSSL(fmt, args...) _log_openssl_err('D', fmt, ## args)
#define L_INFO_OPENSSL(fmt, args...)  _log_openssl_err('I', fmt, ## args)
#define L_WARN_OPENSSL(fmt, args...)  _log_openssl_err('W', fmt, ## args)
#define L_ERROR_OPENSSL(fmt, args...) _log_openssl_err('E', fmt, ## args)
#define L_FATAL_OPENSSL(fmt, args...) _log_openssl_err('F', fmt, ## args)
#define L_AUDIT_OPENSSL(fmt, args...) _log_openssl_err('A', fmt, ## args)

#define L_DEBUG_OPENSSL_P(fmt, args...) _log_openssl_err('D', "%s:%s:%d " fmt, __FILE__, __func__, __LINE__, ## args)
#define L_INFO_OPENSSL_P(fmt, args...)  _log_openssl_err('I', "%s:%s:%d " fmt, __FILE__, __func__, __LINE__, ## args)
#define L_WARN_OPENSSL_P(fmt, args...)  _log_openssl_err('W', "%s:%s:%d " fmt, __FILE__, __func__, __LINE__, ## args)
#define L_ERROR_OPENSSL_P(fmt, args...) _log_openssl_err('E', "%s:%s:%d " fmt, __FILE__, __func__, __LINE__, ## args)
#define L_FATAL_OPENSSL_P(fmt, args...) _log_openssl_err('F', "%s:%s:%d " fmt, __FILE__, __func__, __LINE__, ## args)
#define L_AUDIT_OPENSSL_P(fmt, args...) _log_openssl_err('A', "%s:%s:%d " fmt, __FILE__, __func__, __LINE__, ## args)
*/

void logging_flush();
void logging_open_file(void);
void logging_finish(void);

static inline bool logging_get_verbose(void)
{
	return libsccmn_config.log_verbose;
}

///

struct log_entry
{
	double timestamp;
	pid_t pid;
	char level;
	char message[4096];
};

void log_entry_process(struct log_entry * le, int le_message_length);

#endif //__LIBSCCMN_LOG_H__
