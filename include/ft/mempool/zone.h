#ifndef FT_MEMPOOL_ZONE_H_
#define FT_MEMPOOL_ZONE_H_

struct ft_pool;

///

struct ft_poolzone
{
	struct ft_poolzone * next;

	struct
	{
		unsigned int freeable : 1; // If true, then the zone can be returned (unmmaped) back to OS when all frames are returned
		unsigned int erase_on_return : 1; // If true, frames will be erased using bzero() on ft_frame_return()
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

bool ft_poolzone_init(struct ft_poolzone *, uint8_t * data, size_t alloc_size, size_t frame_count, bool freeable);

struct ft_poolzone * ft_poolzone_new_mmap(size_t frame_count, bool freeable, int mmap_flags);

///

typedef struct ft_poolzone * (* ft_pool_alloc_fnct)(struct ft_pool *);

struct ft_poolzone * ft_pool_alloc_default(struct ft_pool * this);
struct ft_poolzone * ft_pool_alloc_hugetlb(struct ft_pool * this);

#endif // FT_MEMPOOL_ZONE_H_
