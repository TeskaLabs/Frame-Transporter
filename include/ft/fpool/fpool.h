#ifndef FT_FPOOL_H_
#define FT_FPOOL_H_

/*
Fixed-size block memory pool.
See https://en.wikipedia.org/wiki/Memory_pool
*/

struct frame_pool;

struct frame_pool_zone
{
	struct frame_pool_zone * next;

	struct
	{
		unsigned int freeable : 1; // If true, then the zone can be returned (unmmaped) back to OS when all frames are returned
		unsigned int erase_on_return : 1; // If true, frames will be erased using bzero() on frame_pool_return()
		unsigned int mlock_when_used : 1; // If true, frames will be mlock()-ed into memory when in use (prevention of swapping to the disk)
		unsigned int free_on_hb : 1; // If true, zone will be removed during next heartbeat
	} flags;

	size_t alloc_size;
	size_t frames_total;
	size_t frames_used;

	ev_tstamp free_at;	

	struct frame * available_frames;
	struct frame * low_frame;
	struct frame * high_frame;
	struct frame frames[];
};

bool frame_pool_zone_init(struct frame_pool_zone *, uint8_t * data, size_t alloc_size, size_t frame_count, bool freeable);

struct frame_pool_zone * frame_pool_zone_new_mmap(struct frame_pool * frame_pool, size_t frame_count, bool freeable, int mmap_flags);

typedef struct frame_pool_zone * (* frame_pool_zone_alloc_advice)(struct frame_pool *);

struct frame_pool
{
	struct frame_pool_zone * zones;
	frame_pool_zone_alloc_advice alloc_advise;
};

bool frame_pool_init(struct frame_pool *, struct ft_context * context); // Context can be null
void frame_pool_fini(struct frame_pool *);

void frame_pool_set_alloc_advise(struct frame_pool *, frame_pool_zone_alloc_advice alloc_advise);
struct frame_pool_zone * frame_pool_zone_alloc_advice_default(struct frame_pool * this);
struct frame_pool_zone * frame_pool_zone_alloc_advice_hugetlb(struct frame_pool * this);

size_t frame_pool_available_frames_count(struct frame_pool *);
size_t frame_pool_zones_count(struct frame_pool *);


struct frame * frame_pool_borrow_real(struct frame_pool *, uint64_t frame_type, const char * file, unsigned int line);
#define frame_pool_borrow(pool, frame_type) frame_pool_borrow_real(pool, frame_type, __FILE__, __LINE__)

static inline void frame_pool_return(struct frame * frame)
{
	int rc;
	struct frame_pool_zone * zone = frame->zone;

	frame->type = frame_type_FREE;

	frame->next = zone->available_frames;
	zone->available_frames = frame;

	zone->frames_used -= 1;
	zone->flags.free_on_hb = (frame->zone->frames_used == 0);

	// Erase a page
	if (zone->flags.erase_on_return)
		bzero(frame->data, frame->capacity);

	// Unlock pages from the memory
	if (zone->flags.mlock_when_used)
	{
		rc = munlock(frame->data, frame->capacity);
		if (rc != 0) FT_WARN_ERRNO(errno, "munlock in frame pool return");
	}

	// Advise that we will use it
	rc = posix_madvise(frame->data, frame->capacity, POSIX_MADV_DONTNEED);
	if (rc != 0) FT_WARN_ERRNO(errno, "posix_madvise in frame pool return");

}

#endif // FT_FPOOL_H_
