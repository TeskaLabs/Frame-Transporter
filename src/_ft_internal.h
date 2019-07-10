#include <ft.h>
#include <openssl/rand.h>
//#include <sys/mman.h>

// Frame size should be above 16kb, which is a maximum record size of 16kB for SSLv3/TLSv1
// See https://www.openssl.org/docs/manmaster/ssl/SSL_read.html
#define FRAME_SIZE (5*4096)
#define MEMPAGE_SIZE (4096)

// Fake mmap parameters on Windows
#define MAP_PRIVATE 0
#define MAP_ANON 0

void ft_log_initialise_(void);

void _ft_frame_init(struct ft_frame * , uint8_t * data, size_t capacity, struct ft_poolzone * zone);

// Package private pool zone methods
void _ft_poolzone_del(struct ft_poolzone * );
struct ft_frame * _ft_poolzone_borrow(struct ft_poolzone * , uint64_t frame_type, const char * file, unsigned int line);
