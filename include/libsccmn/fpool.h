#ifndef __LIBSCCMN_FPOOL_H__
#define __LIBSCCMN_FPOOL_H__

/*
Fixed-size block memory pool.
See https://en.wikipedia.org/wiki/Memory_pool
*/

struct frame_pool;

struct frame_pool_zone
{
	struct frame_pool_zone * next;

	bool freeable; // If true, then the zone can be returned (unmmaped) back to OS when all frames are returned
	size_t frames_total;
	size_t frames_used;

	bool free_timeout_triggered;
	ev_tstamp free_at;

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
	struct heartbeat_watcher heartbeat_w;
};

bool frame_pool_init(struct frame_pool *, struct heartbeat * heartbeat);
void frame_pool_fini(struct frame_pool *, struct heartbeat * heartbeat);

void frame_pool_set_alloc_advise(struct frame_pool *, frame_pool_zone_alloc_advice alloc_advise);


struct frame * frame_pool_borrow_real(struct frame_pool *, uint64_t frame_type, const char * file, unsigned int line);
#define frame_pool_borrow(pool, frame_type) frame_pool_borrow_real(pool, frame_type, __FILE__, __LINE__)

static inline void frame_pool_return(struct frame * frame)
{
	struct frame_pool_zone * zone = frame->zone;

	frame->type = frame_type_FREE;

	frame->next = zone->available_frames;
	zone->available_frames = frame;

	zone->frames_used -= 1;
	zone->free_timeout_triggered = (frame->zone->frames_used == 0);

	// Erase a page
	// TODO: Conditionally based on the zone configuration
	bzero(frame->data, frame->capacity);
}

#endif //__LIBSCCMN_FPOOL_H__
