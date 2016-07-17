#ifndef FT_MEMPOOL_POOL_H_
#define FT_MEMPOOL_POOL_H_

/*
Fixed-size block memory pool.
See https://en.wikipedia.org/wiki/Memory_pool
*/

struct frame_pool
{
	struct ft_poolzone * zones;
	ft_pool_alloc_fnct alloc_fnct;
};

bool frame_pool_init(struct frame_pool *, struct ft_context * context); // Context can be null
void frame_pool_fini(struct frame_pool *);

void ft_pool_set_alloc(struct frame_pool *, ft_pool_alloc_fnct alloc_fnct);

size_t frame_pool_available_frames_count(struct frame_pool *);
size_t frame_pool_zones_count(struct frame_pool *);

struct frame * frame_pool_borrow_real(struct frame_pool *, uint64_t frame_type, const char * file, unsigned int line);
#define frame_pool_borrow(pool, frame_type) frame_pool_borrow_real(pool, frame_type, __FILE__, __LINE__)

static inline void frame_pool_return(struct frame * frame)
{
	int rc;
	struct ft_poolzone * zone = frame->zone;

	frame->type = FT_FRAME_TYPE_FREE;

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

#endif // FT_MEMPOOL_POOL_H_
