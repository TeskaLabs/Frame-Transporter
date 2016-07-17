#ifndef FT_H_
#define FT_H_

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
struct ft_context;

#include <ft/cols/list.h>

#include <ft/config.h>
#include <ft/log.h>
#include <ft/ini.h>
#include <ft/heartbeat.h>
#include <ft/frame.h>
#include <ft/fpool.h>
#include <ft/context.h>

#include <ft/sock/listener.h>
#include <ft/sock/stream.h>

// Global init function
void ft_initialise(void); // Call this at very beginning

pid_t ft_deamonise(struct ft_context * context); // context can be NULL

void ft_pidfile_filename(const char * fname);
bool ft_pidfile_create(void);
bool ft_pidfile_remove(void);
pid_t ft_pidfile_is_running(void);


// File descriptor related functions
bool ft_fd_nonblock(int fd);
bool ft_socket_keepalive(int fd);
bool ft_fd_cloexec(int fd);


// Boolean parser
bool ft_parse_bool(const char * value);


/*
 * Fills *buf with max. *buflen characters, encoding size bytes of *data.
 *
 * NOTE: *buf space should be at least 1 byte _more_ than *buflen
 * to hold the trailing '\0'.
 *
 * return value    : #bytes filled in buf   (excluding \0)
 * sets *buflen to : #bytes encoded from data
 */
int ft_base32_encode(char * buf, size_t * buflen, const void * data, size_t size);

#endif // FT_H_
