#include <ft.h>
#include <openssl/rand.h>
#include <sys/mman.h>

// Frame size should be above 16kb, which is a maximum record size of 16kB for SSLv3/TLSv1
// See https://www.openssl.org/docs/manmaster/ssl/SSL_read.html
#define FRAME_SIZE (5*4096)
#define MEMPAGE_SIZE (4096)

void _ft_log_initialise(void);

void _frame_init(struct frame * this, uint8_t * data, size_t capacity, struct frame_pool_zone * zone);
