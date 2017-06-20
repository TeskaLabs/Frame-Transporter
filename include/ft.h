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
#include <stdatomic.h>
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
#include <sys/un.h>
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
#include <ft/cols/iphashmap.h>
#include <ft/cols/stack.h>

#include <ft/config.h>
#include <ft/log.h>
#include <ft/ini.h>
#include <ft/pubsub.h>
#include <ft/loadstore.h>
#include <ft/mempool/vec.h>
#include <ft/mempool/frame.h>
#include <ft/mempool/zone.h>
#include <ft/mempool/pool.h>
#include <ft/context.h>

#include <ft/sock/socket.h>
#include <ft/sock/listener.h>
#include <ft/sock/stream.h>
#include <ft/sock/dgram.h>
#include <ft/sock/uni.h>

#include <ft/proto/socks.h>

// Global init function
void ft_initialise(void); // Call this at very beginning

pid_t ft_daemonise(struct ft_context * context); // context can be NULL

void ft_pidfile_filename(const char * fname);
bool ft_pidfile_create(void);
bool ft_pidfile_remove(void);
pid_t ft_pidfile_is_running(void);


// File descriptor related functions
bool ft_fd_nonblock(int fd, bool nonblock);
bool ft_socket_keepalive(int fd);
bool ft_fd_cloexec(int fd);

// Can be safely called with NULL in the context
ev_tstamp ft_safe_now(struct ft_context *);

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


/* Helper macro that declares and initialize a struct addrinfo from UNIX address (file system path)
 * Usage:
 *
 * FT_ADDR_UNIX(addr, "/tmp/unix.socket", SOCK_STREAM);
 * ft_stream_connect(&stream, &stream_delegate, context, &addr);
 *
 */

#define FT_ADDR_UNIX(NAME, PATH, SOCKTYPE) \
	struct sockaddr_un NAME_saddr_ = { .sun_family = AF_UNIX }; \
	struct addrinfo NAME; \
	{ \
		strcpy(NAME_saddr_.sun_path, PATH); \
		memset(&addr, 0, sizeof(struct addrinfo)); \
		addr.ai_family = AF_UNIX; \
		addr.ai_socktype = SOCKTYPE; \
		addr.ai_flags = 0; \
		addr.ai_protocol = 0; \
		addr.ai_canonname = NULL; \
		addr.ai_addrlen = SUN_LEN(&NAME_saddr_); \
		addr.ai_addr = (struct sockaddr *)&NAME_saddr_; \
		addr.ai_next = NULL; \
	}

bool ft_sockaddr_hostport(const struct sockaddr * client_addr, socklen_t client_addr_len, char * host, socklen_t hostlen, char * port, socklen_t portlen);
const char * ft_sockaddr_hostport_str(const struct sockaddr * client_addr, socklen_t client_addr_len);

#endif // FT_H_
