#include "all.h"

struct frame_pool_zone * frame_pool_zone_new(size_t frame_count, bool freeable)
{
	assert(frame_count > 0);

	size_t mmap_size_frames = frame_count * FRAME_SIZE;
	assert((mmap_size_frames % MEMPAGE_SIZE) == 0);

	size_t mmap_size_zone = sizeof(struct frame_pool_zone) + frame_count * sizeof(struct frame);
	size_t mmap_size_fill = MEMPAGE_SIZE - (mmap_size_zone % MEMPAGE_SIZE);
	if (mmap_size_fill == MEMPAGE_SIZE) mmap_size_fill = 0;
	assert(((mmap_size_frames+mmap_size_zone+mmap_size_fill) % MEMPAGE_SIZE) == 0);

	void * p = mmap(NULL, mmap_size_frames+mmap_size_zone+mmap_size_fill, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
	if (p == MAP_FAILED)
	{
		L_ERROR_ERRNO(errno, "Failed to allocate frame pool memory (%zu bytes)", mmap_size_frames+mmap_size_zone+mmap_size_fill);
		return NULL;
	}

	struct frame_pool_zone * this = p;
	this->flags.freeable = freeable;
	this->flags.erase_on_return = true;
	this->flags.mlock_when_used = true;
	this->flags.free_on_hb = false;
	this->mmap_size = mmap_size_frames+mmap_size_zone+mmap_size_fill;
	this->next = NULL;

	this->frames_total = frame_count;
	this->frames_used = 0;

	this->low_frame = &this->frames[0];
	this->high_frame = &this->frames[this->frames_total-1];

	uint8_t * frame_data = p + (mmap_size_zone + mmap_size_fill);
	for(int i=0; i<this->frames_total; i+=1)
	{
		struct frame * frame = &this->frames[i];
		_frame_init(frame, frame_data + (i*FRAME_SIZE), FRAME_SIZE, this);
		assert(frame >= this->low_frame);
		assert(frame <= this->high_frame);
	}

	// Prepare stack (S-list) of available (aka all at this time) frames in this zone
	this->available_frames = this->low_frame;
	for(int i=1; i<this->frames_total; i+=1)
		this->frames[i-1].next = &this->frames[i];
	this->high_frame->next = NULL;

	L_DEBUG("Allocated frame pool zone of %zu bytes, %zd frames", this->mmap_size, this->frames_total);
	return this;
}

static void frame_pool_zone_del(struct frame_pool_zone * this)
{
	assert(this != NULL);
	assert(this->frames_used >= 0);

	if (this->frames_used > 0)
	{
		L_FATAL("Not all frames are returned to the pool during frame pool zone destruction (count of unreturned frames: %zd)", this->frames_used);
		for(int i=0; i<this->frames_total; i+=1)
		{
			if (this->frames[i].type != frame_type_FREE)
			{
				L_FATAL("Unreturned frame #%d allocated at %s:%d", i+1, this->frames[i].borrowed_by_file, this->frames[i].borrowed_by_line);
			}
		}
		abort();
	}

	L_DEBUG("Deallocating frame pool zone of %zu bytes", this->mmap_size);
	munmap(this, this->mmap_size);
}


static struct frame * frame_pool_zone_borrow(struct frame_pool_zone * this, uint64_t frame_type, const char * file, unsigned int line)
{
	int rc;

	if (this->available_frames == NULL) return NULL; // Zone has no available frames
	struct frame * frame = this->available_frames;
	this->available_frames = frame->next;

	// Reset frame
	frame->next = NULL;
	frame->type = frame_type;
	frame->borrowed_by_file = file;
	frame->borrowed_by_line = line;
	frame->zone->frames_used += 1;

	frame->dvec_position = 0;
	frame->dvec_limit = 0;

	// Lock the frame the memory
	if (frame->zone->flags.mlock_when_used)
	{
		rc = mlock(frame->data, frame->capacity);
		if (rc != 0) L_WARN_ERRNO(errno, "mlock in frame pool borrow");
	}

	// Advise that we will use it
	rc = posix_madvise(frame->data, frame->capacity, POSIX_MADV_WILLNEED);
	if (rc != 0) L_WARN_ERRNO(errno, "posix_madvise in frame pool borrow");

	return frame;
}

//

static void frame_pool_heartbeat_cb(struct heartbeat_watcher * watcher, struct heartbeat * heartbeat, ev_tstamp now);
static struct frame_pool_zone * frame_pool_zone_alloc_advice_default(struct frame_pool * this);


bool frame_pool_init(struct frame_pool * this, struct heartbeat * heartbeat)
{
	assert(this != NULL);
	this->zones = NULL;
	
	heartbeat_add(heartbeat, &this->heartbeat_w, frame_pool_heartbeat_cb);
	this->heartbeat_w.data = this;

	this->alloc_advise = frame_pool_zone_alloc_advice_default;

	return true;
}


void frame_pool_fini(struct frame_pool * this, struct heartbeat * heartbeat)
{
	assert(this != NULL);

	while(this->zones != NULL)
	{
		struct frame_pool_zone * zone = this->zones;
		this->zones = zone->next;

		frame_pool_zone_del(zone);
	}

	heartbeat_remove(heartbeat, &this->heartbeat_w);
}


struct frame * frame_pool_borrow_real(struct frame_pool * this, uint64_t frame_type, const char * file, unsigned int line)
{
	assert(this != NULL);
	struct frame * frame =  NULL;
	struct frame_pool_zone ** last_zone_next = &this->zones;

	// Borrow from existing zone
	for (struct frame_pool_zone * zone = this->zones; zone != NULL; zone = zone->next)
	{
		frame = frame_pool_zone_borrow(zone, frame_type, file, line);
		if (frame != NULL) return frame;

		last_zone_next = &zone->next;
	}

	*last_zone_next = this->alloc_advise(this);
	if (*last_zone_next == NULL)
	{
		L_WARN("Frame pool ran out of memory");
		return NULL;
	}

	frame = frame_pool_zone_borrow(*last_zone_next, frame_type, file, line);
	if (frame == NULL)
	{
		L_WARN("Frame pool ran out of memory (2)");
		return NULL;
	}

	return frame;
}


struct frame_pool_zone * frame_pool_zone_alloc_advice_default(struct frame_pool * this)
{
	if (this->zones == NULL) return frame_pool_zone_new(16, false); // Allocate first, low-memory zone
	if (this->zones->next == NULL) return frame_pool_zone_new(2045, true); // Allocate second, high-memory zone
	return NULL;
}


void frame_pool_set_alloc_advise(struct frame_pool * this, frame_pool_zone_alloc_advice alloc_advise)
{
	if (alloc_advise == NULL) this->alloc_advise = frame_pool_zone_alloc_advice_default;
	else this->alloc_advise = alloc_advise;
}


void frame_pool_heartbeat_cb(struct heartbeat_watcher * watcher, struct heartbeat * heartbeat, ev_tstamp now)
{
	struct frame_pool * this = watcher->data;
	assert(this != NULL);

	// Iterate via zones and find free-able ones with no used frames ...
	struct frame_pool_zone ** last_zone_next = &this->zones;
	struct frame_pool_zone * zone = this->zones;
	while (zone != NULL)
	{
		if ((!zone->flags.freeable) || (zone->frames_used > 0))
		{
			last_zone_next = &zone->next;
			zone = zone->next;
			continue;
		}

		if (zone->flags.free_on_hb)
		{
			zone->free_at = now + libsccmn_config.fpool_zone_free_timeout;
			zone->flags.free_on_hb = false;

			last_zone_next = &zone->next;
			zone = zone->next;
			continue;
		}

		if (zone->free_at >= now)
		{
			last_zone_next = &zone->next;
			zone = zone->next;
			continue;			
		}

		// Unchain the zone
		struct frame_pool_zone * zone_free = zone;
		zone = zone->next;
		*last_zone_next = zone;

		// Delete zone
		frame_pool_zone_del(zone_free);
	}
}
