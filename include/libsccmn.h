#ifndef __LIBSCCMN_H__
#define __LIBSCCMN_H__

#if defined(__linux)
#define _GNU_SOURCE 1
//See: http://stackoverflow.com/questions/5582211/what-does-define-gnu-source-imply
//Defining _GNU_SOURCE has nothing to do with license and everything to do with writing (non-)portable code.
#endif

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <limits.h>
#include <time.h>
#include <fcntl.h>
#include <assert.h>
#include <errno.h>
#include <regex.h>
#include <math.h>
#include <ctype.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <netdb.h>

// Include libev
#include <ev.h>

// Include OpenSSL
#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>


// Forward declarations
struct context;

#include <libsccmn/config.h>
#include <libsccmn/log.h>
#include <libsccmn/ini.h>
#include <libsccmn/heartbeat.h>
#include <libsccmn/frame.h>
#include <libsccmn/fpool.h>
#include <libsccmn/context.h>

#include <libsccmn/cols/list.h>

#include <libsccmn/sock_listen.h>
#include <libsccmn/sock_est.h>

// Global init function
void ft_initialise(void); // Call this at very beginning

// ft_deamonise functions
// context can be NULL
pid_t ft_deamonise(struct context * context);

void pidfile_set_filename(const char * fname);
bool pidfile_create(void);
bool pidfile_remove(void);
pid_t pidfile_is_running(void);


// File descriptor related functions
bool set_socket_nonblocking(int fd);
bool set_tcp_keepalive(int fd);
bool set_close_on_exec(int fd);


// Boolean parser
bool parse_boolean(const char * value);


/*
 * Fills *buf with max. *buflen characters, encoding size bytes of *data.
 *
 * NOTE: *buf space should be at least 1 byte _more_ than *buflen
 * to hold the trailing '\0'.
 *
 * return value    : #bytes filled in buf   (excluding \0)
 * sets *buflen to : #bytes encoded from data
 */
int base32_encode(char * buf, size_t * buflen, const void * data, size_t size);

#endif // __LIBSCCMN_H__
