#ifndef __LIBSCCMN_H__
#define __LIBSCCMN_H__

#if defined(__linux)
#define _GNU_SOURCE 1
//See: http://stackoverflow.com/questions/5582211/what-does-define-gnu-source-imply
//Defining _GNU_SOURCE has nothing to do with license and everything to do with writing (non-)portable code.
#endif

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <limits.h>
#include <time.h>
#include <fcntl.h>
#include <assert.h>
#include <netdb.h>
#include <errno.h>
#include <regex.h>
#include <math.h>
#include <ctype.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <sys/wait.h>

#include <libsccmn/config.h>
#include <libsccmn/ini.h>



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
