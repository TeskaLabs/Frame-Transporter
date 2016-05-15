#include <libsccmn.h>
#include <openssl/rand.h>
#include <sys/mman.h>

#define FRAME_SIZE (3*4096)
#define MEMPAGE_SIZE (4096)

void _logging_init(void);

void _frame_init(struct frame * this, uint8_t * data, struct frame_pool_zone * zone);
