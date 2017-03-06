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

// Default pool (will be set by first ft_pool_init() call ... or you can override it)
// It removes need for complicated pool localisation in the code
extern struct ft_pool * ft_pool_default;

bool ft_pool_init(struct ft_pool *); // Context can be null
void ft_pool_fini(struct ft_pool *);

void ft_pool_set_alloc(struct ft_pool *, ft_pool_alloc_fnct alloc_fnct);

size_t ft_pool_count_available_frames(struct ft_pool *);
size_t ft_pool_count_zones(struct ft_pool *);

struct ft_frame * ft_pool_borrow_real_(struct ft_pool *, uint64_t frame_type, const char * file, unsigned int line);
#define ft_pool_borrow(pool, frame_type) ft_pool_borrow_real_(pool, frame_type, __FILE__, __LINE__)
#define ft_frame_borrow(frame_type) ft_pool_borrow_real_(NULL, frame_type, __FILE__, __LINE__)

// Following function requires access to pool object internals
// and for this reason it is here and not with ft_frame object
static inline void ft_frame_return(struct ft_frame * frame)
{
	int rc;

	assert(frame != NULL);

	FT_TRACE(FT_TRACE_ID_MEMPOOL, "BEGIN f:%p ft:%x fb:%s:%u", frame, frame->type, frame->borrowed_by_file, frame->borrowed_by_line);

	assert(frame->type != FT_FRAME_TYPE_FREE);
	assert(frame->borrowed_by_file != NULL);
	assert(frame->borrowed_by_line > 0);

	struct ft_poolzone * zone = frame->zone;
	frame->type = FT_FRAME_TYPE_FREE;

	frame->next = zone->available_frames;
	zone->available_frames = frame;

	frame->borrowed_by_file = NULL;
	frame->borrowed_by_line = 0;

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
	if (zone->flags.madvice_when_used)
	{
		rc = posix_madvise(frame->data, frame->capacity, POSIX_MADV_DONTNEED);
		if (rc != 0) FT_WARN_ERRNO(errno, "posix_madvise in frame pool return");
	}

	FT_TRACE(FT_TRACE_ID_MEMPOOL, "END");
}

// Call this peridically (ft_context do that for default pool)
void ft_pool_heartbeat(struct ft_pool *, ev_tstamp now);

#endif // FT_MEMPOOL_POOL_H_
