#include "../_ft_internal.h"

///

bool ft_poolzone_init(struct ft_poolzone * this, uint8_t * data, size_t alloc_size, size_t frame_count, bool freeable)
{
	assert(this != NULL);
	assert(frame_count > 0);
	assert((void *)this < (void *)data);
	assert(((void *)this + alloc_size) > (void *)data);

	this->flags.freeable = freeable;
	this->flags.erase_on_return = false;
	this->flags.mlock_when_used = false;
	this->flags.free_on_hb = false;
	this->flags.madvice_when_used = false;
	this->alloc_size = alloc_size;
	this->next = NULL;

	this->frames_total = frame_count;
	this->frames_used = 0;

	this->low_frame = &this->frames[0];
	this->high_frame = &this->frames[this->frames_total-1];

	uint8_t * frame_data = data;
	for(int i=0; i<this->frames_total; i+=1)
	{
		struct ft_frame * frame = &this->frames[i];
		_ft_frame_init(frame, frame_data + (i*FRAME_SIZE), FRAME_SIZE, this);
		assert(frame >= this->low_frame);
		assert(frame <= this->high_frame);
	}

	// Prepare stack (S-list) of available (aka all at this time) frames in this zone
	this->available_frames = this->low_frame;
	for(int i=1; i<this->frames_total; i+=1)
		this->frames[i-1].next = &this->frames[i];
	this->high_frame->next = NULL;

	FT_DEBUG("Allocated frame pool zone of %zu bytes, %zd frames", this->alloc_size, this->frames_total);

	return true;
}

void _ft_poolzone_del(struct ft_poolzone * this)
{
	assert(this != NULL);
	assert(this->frames_used >= 0);

	if (this->frames_used > 0)
	{
		FT_FATAL("Not all frames are returned to the pool during frame pool zone destruction (count of unreturned frames: %zd)", this->frames_used);
		for(int i=0; i<this->frames_total; i+=1)
		{
			if (this->frames[i].type != FT_FRAME_TYPE_FREE)
			{
				FT_FATAL("Unreturned frame #%d allocated at %s:%d", i+1, this->frames[i].borrowed_by_file, this->frames[i].borrowed_by_line);
			}
		}
		abort();
	}

	FT_DEBUG("Deallocating frame pool zone of %zu bytes", this->alloc_size);

	//TODO: configure actual free() call; rename _ft_poolzone_del to frame_pool_zone_fini and implement virtual frame_pool_zone_fini
	munmap(this, this->alloc_size);
}


struct ft_poolzone * ft_poolzone_new_mmap(size_t frame_count, bool freeable, int mmap_flags)
{
	size_t mmap_size_frames = frame_count * FRAME_SIZE;
	assert((mmap_size_frames % MEMPAGE_SIZE) == 0);

	size_t mmap_size_zone = sizeof(struct ft_poolzone) + frame_count * sizeof(struct ft_frame);
	size_t mmap_size_fill = MEMPAGE_SIZE - (mmap_size_zone % MEMPAGE_SIZE);
	if (mmap_size_fill == MEMPAGE_SIZE) mmap_size_fill = 0;
	size_t alloc_size = mmap_size_frames + mmap_size_zone + mmap_size_fill;
	assert((alloc_size % MEMPAGE_SIZE) == 0);
	

	void * p = mmap(NULL, alloc_size, PROT_READ | PROT_WRITE, mmap_flags, -1, 0);
	if (p == MAP_FAILED)
	{
		FT_ERROR_ERRNO(errno, "Failed to allocate frame pool memory (%zu bytes)", alloc_size);
		return NULL;
	}

	struct ft_poolzone * this = p;
	uint8_t * data = p + (mmap_size_zone + mmap_size_fill);

	bool ok = ft_poolzone_init(this, data, alloc_size, frame_count, freeable);
	if (!ok)
	{
		munmap(p, alloc_size);
		return NULL;
	}

	return this;
}


struct ft_frame * _ft_poolzone_borrow(struct ft_poolzone * this, uint64_t frame_type, const char * file, unsigned int line)
{
	int rc;

	if (this->available_frames == NULL) return NULL; // Zone has no available frames
	struct ft_frame * frame = this->available_frames;
	this->available_frames = frame->next;

	// Reset frame
	frame->next = NULL;
	frame->type = frame_type;
	frame->borrowed_by_file = file;
	frame->borrowed_by_line = line;
	frame->zone->frames_used += 1;

	frame->vec_position = 0;
	frame->vec_limit = 0;

	// Lock the frame the memory
	if (frame->zone->flags.mlock_when_used)
	{
		rc = mlock(frame->data, frame->capacity);
		if (rc != 0) FT_WARN_ERRNO(errno, "mlock in frame pool borrow");
	}

	// Advise that we will use it
	if (frame->zone->flags.madvice_when_used)
	{
		rc = posix_madvise(frame->data, frame->capacity, POSIX_MADV_WILLNEED);
		if (rc != 0) FT_WARN_ERRNO(errno, "posix_madvise in frame pool borrow");
	}

	return frame;
}

/// Zone allocators follows ...

struct ft_poolzone * ft_pool_alloc_default(struct ft_pool * this)
{
	if (this->zones == NULL) return ft_poolzone_new_mmap(16, false,  MAP_PRIVATE | MAP_ANON); // Allocate first, low-memory zone
	if (this->zones->next == NULL) return ft_poolzone_new_mmap(2045, true,  MAP_PRIVATE | MAP_ANON); // Allocate second, high-memory zone
	return NULL;
}


#ifdef MAP_HUGETLB
struct ft_poolzone * ft_pool_alloc_hugetlb(struct ft_pool * this)
{
	if (this->zones == NULL)
	{
		char buffer[2048];
		int fd = open("/proc/meminfo", O_RDONLY);
		if (fd < 0)
		{
			FT_WARN_ERRNO(errno, "open(%s), disabling hugetlb support", "/proc/meminfo");
			goto cont_default;
		}

		int len = read(fd, buffer, sizeof(buffer));
		int readerr = errno;
		close(fd);

		if (len < 0)
		{
			FT_ERROR_ERRNO(readerr, "read(%s), disabling hugetlb support", "/proc/meminfo");
			goto cont_default;
		}
		if (len == sizeof(buffer))
		{
			FT_ERROR("File '%s' is too large, disabling hugetlb support", "/proc/meminfo");
			goto cont_default;
		}
		buffer[len] = '\0';

		char * p = strstr(buffer, "Hugepagesize:");
		if (p == NULL)
		{
			FT_ERROR("File '%s' doesn't contain hugetlb page size, disabling hugetlb support", "/proc/meminfo");
			goto cont_default;
		}
		p += strlen("Hugepagesize:");

		char * q;
		long hugetlb_size = strtol(p, &q, 0);
		if (!isspace(*q))
		{
			FT_ERROR("File '%s' parsing error, disabling hugetlb support", "/proc/meminfo");
			goto cont_default;
		}
		hugetlb_size *= 1024;
		int frame_count = (hugetlb_size - MEMPAGE_SIZE) / FRAME_SIZE;

		size_t alloc_size;
		frame_count += 1;
		do {
			frame_count -= 1;
			size_t mmap_size_frames = frame_count * FRAME_SIZE;
			size_t mmap_size_zone = sizeof(struct ft_poolzone) + frame_count * sizeof(struct ft_frame);
			size_t mmap_size_fill = MEMPAGE_SIZE - (mmap_size_zone % MEMPAGE_SIZE);
			if (mmap_size_fill == MEMPAGE_SIZE) mmap_size_fill = 0;
			alloc_size = mmap_size_frames + mmap_size_zone + mmap_size_fill;
		} while (alloc_size > hugetlb_size);

		FT_DEBUG("Hugetlb page will be used for frame pool zone (huge table size: %ld)", hugetlb_size);

		struct ft_poolzone * ret = ft_poolzone_new_mmap(frame_count, false,  MAP_PRIVATE | MAP_ANON | MAP_HUGETLB);
		if (ret != NULL) return ret;
		FT_WARN("Hugetlb support disabled");
	}

cont_default:
	return ft_pool_alloc_default(this);
}
#else
struct ft_poolzone * ft_pool_alloc_hugetlb(struct ft_pool * this)
{
	FT_DEBUG("Huge table frame pool allocator is not available");
	return ft_pool_alloc_default(this);
}
#endif
