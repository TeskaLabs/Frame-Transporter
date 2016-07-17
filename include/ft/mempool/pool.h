#ifndef FT_MEMPOOL_POOL_H_
#define FT_MEMPOOL_POOL_H_

/*
Fixed-size block memory pool.
See https://en.wikipedia.org/wiki/Memory_pool
*/

struct ft_pool
{
	struct ft_poolzone * zones;
	ft_pool_alloc_fnct alloc_fnct;
};

bool ft_pool_init(struct ft_pool *, struct ft_context * context); // Context can be null
void ft_pool_fini(struct ft_pool *);

void ft_pool_set_alloc(struct ft_pool *, ft_pool_alloc_fnct alloc_fnct);

size_t ft_pool_count_available_frames(struct ft_pool *);
size_t ft_pool_count_zones(struct ft_pool *);

struct frame * _ft_pool_borrow_real(struct ft_pool *, uint64_t frame_type, const char * file, unsigned int line);
#define ft_pool_borrow(pool, frame_type) _ft_pool_borrow_real(pool, frame_type, __FILE__, __LINE__)

// Following function requires access to pool object internals
// and for this reason it is here and not with ft_frame object
static inline void ft_frame_return(struct frame * frame)
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
