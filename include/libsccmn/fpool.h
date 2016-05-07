#ifndef __LIBSCCMN_FPOOL_H__
#define __LIBSCCMN_FPOOL_H__

/*
Fixed-size block memory pool.
See https://en.wikipedia.org/wiki/Memory_pool
*/

struct frame_pool;

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

enum frame_types
{
	frame_type_FREE    = 0xFFFFFFFF,
	frame_type_UNKNOWN = 0xFFFFFFEE
};

struct frame
{
	struct frame * next; // This allows to chain frames in the list
	struct frame_pool_zone * zone;

	enum frame_types type;
	struct frame_dvec * dvecs;

	// Those two are used for tracing and debugging 
	const char * borrowed_by_file;
	unsigned int borrowed_by_line;

	uint8_t * data;
};


struct frame_pool_zone
{
	struct frame_pool_zone * next;

	bool freeable; // If true, then the zone can be returned (unmmaped) back to OS when all frames are returned
	size_t frames_total;
	size_t frames_used;

	size_t mmap_size;

	struct frame * available_frames;

	struct frame * low_frame;
	struct frame * high_frame;
	struct frame frames[];
};

struct frame_pool_zone * frame_pool_zone_new(size_t frame_count, bool freeable);


typedef struct frame_pool_zone * (* frame_pool_zone_alloc_advice)(struct frame_pool *);

struct frame_pool
{
	struct frame_pool_zone * zones;
	frame_pool_zone_alloc_advice alloc_advise;
};

bool frame_pool_init(struct frame_pool *, frame_pool_zone_alloc_advice alloc_advise);
void frame_pool_fini(struct frame_pool *);

struct frame * frame_pool_borrow_real(struct frame_pool *, const char * file, unsigned int line);
#define frame_pool_borrow(pool) frame_pool_borrow_real(pool, __FILE__, __LINE__)

static inline void frame_pool_return(struct frame * frame)
{
	frame->next = frame->zone->available_frames;
	frame->zone->available_frames = frame;
	frame->zone->frames_used -= 1;

	frame->type = frame_type_FREE;
}

#endif //__LIBSCCMN_FPOOL_H__
