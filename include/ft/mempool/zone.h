#ifndef FT_MEMPOOL_ZONE_H_
#define FT_MEMPOOL_ZONE_H_

struct ft_pool;

///

struct ft_poolzone
{
	struct ft_poolzone * next;

	struct
	{
		bool freeable : 1; // If true, then the zone can be returned (unmmaped) back to OS when all frames are returned
		bool erase_on_return : 1; // If true, frames will be erased using bzero() on ft_frame_return()
		bool mlock_when_used : 1; // If true, frames will be mlock()-ed into memory when in use (prevention of swapping to the disk)
		bool madvice_when_used : 1; // If true, posix_madvise(POSIX_MADV_WILLNEED) resp. posix_madvise(POSIX_MADV_DONTNEED) will be called when frame is borrowed/returned
		bool free_on_hb : 1; // If true, zone will be removed during next heartbeat
	} flags;

	size_t alloc_size;
	size_t frames_total;
	size_t frames_used;

	ev_tstamp free_at;	

	struct ft_frame * available_frames;
	struct ft_frame * low_frame;
	struct ft_frame * high_frame;
	struct ft_frame frames[];
};

bool ft_poolzone_init(struct ft_poolzone *, uint8_t * data, size_t alloc_size, size_t frame_count, bool freeable);

struct ft_poolzone * ft_poolzone_new_mmap(size_t frame_count, bool freeable, int mmap_flags);

///

typedef struct ft_poolzone * (* ft_pool_alloc_fnct)(struct ft_pool *);

struct ft_poolzone * ft_pool_alloc_default(struct ft_pool * this);
struct ft_poolzone * ft_pool_alloc_hugetlb(struct ft_pool * this);

#endif // FT_MEMPOOL_ZONE_H_
