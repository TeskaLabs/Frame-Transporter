#ifndef FT_MEMPOOL_POOL_H_
#define FT_MEMPOOL_POOL_H_

#define FT_POOL_LOWMEM_LIMIT (100)

/*
Fixed-size block memory pool.
See https://en.wikipedia.org/wiki/Memory_pool
*/

struct ft_pool
{
	struct ft_poolzone * zones;
	ft_pool_alloc_fnct alloc_fnct;
	struct ft_subscriber heartbeat;

	struct
	{
		bool last_zone : 1; // The last zone has been allocated,
	} flags;

	int frames_total; // If flags.last_zone is not true, this number could increase via new zone allocation
	int frames_used;
};

bool ft_pool_init(struct ft_pool *); // Context can be null
void ft_pool_fini(struct ft_pool *);

void ft_pool_set_alloc(struct ft_pool *, ft_pool_alloc_fnct alloc_fnct);

size_t ft_pool_count_available_frames(struct ft_pool *);
size_t ft_pool_count_zones(struct ft_pool *);

// This is package private method to send lowmem pubsub message
void ft_pool_lowmem_pubsub_msg_(struct ft_pool * this, int frames_available, bool inc);

struct ft_frame * ft_pool_borrow_real_(struct ft_pool *, uint64_t frame_type, const char * file, unsigned int line);
#define ft_pool_borrow(pool, frame_type) ft_pool_borrow_real_(pool, frame_type, __FILE__, __LINE__)
#define ft_frame_borrow(frame_type) ft_pool_borrow_real_(NULL, frame_type, __FILE__, __LINE__)

// Following function requires access to pool object internals
// and for this reason it is here and not with ft_frame object
void ft_frame_return(struct ft_frame * frame);

#endif // FT_MEMPOOL_POOL_H_
