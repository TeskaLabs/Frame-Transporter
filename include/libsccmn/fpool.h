#ifndef __LIBSCCMN_FPOOL_H__
#define __LIBSCCMN_FPOOL_H__

struct frame_dvec
{
	uint8_t * base;

	// 0 <= position <= limit <= capacity

	// The position is the index of the next byte to be read or written.
	// The position is never negative and is never greater than its limit.
	size_t position;

	// The limit is the index of the first byte that should not be read or written.
	// The limit is never negative and is never greater than its capacity.
	size_t limit;

	// The capacity is the number of bytes it contains.
	// The capacity is never negative and never changes.
	size_t capacity;
};


struct frame
{
	uint32_t type; // Type of frame: 0xFFFFFFFF = unknown, SPDY tags otherwise, 0 = DATA frame
	struct frame_pool_zone * zone;
	struct frame * next_available;

	struct frame_dvec * dvecs;

	const char * borrowed_by_file;
	unsigned int borrowed_by_line;

	uint8_t * data;
};


struct frame_pool_zone
{
	struct frame_pool_zone * next;
	struct
	{
		unsigned extended:1;
	} _flags;

	size_t mmap_size;

	struct frame * available_frames;

	struct frame * low_frame;
	struct frame * high_frame;
	struct frame frames[];
};


struct frame_pool
{
	struct frame_pool_zone * zones;
};

bool frame_pool_init(struct frame_pool *, size_t initial_frame_count);
void frame_pool_fini(struct frame_pool *);

struct frame * frame_pool_borrow_real(struct frame_pool *, const char * file, unsigned int line);
#define frame_pool_borrow(pool) frame_pool_borrow_real(pool, __FILE__, __LINE__)

#endif //__LIBSCCMN_FPOOL_H__
