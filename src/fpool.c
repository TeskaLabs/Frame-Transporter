#include "all.h"

#define FRAME_SIZE (3*4096)
#define MEMPAGE_SIZE (4096)

///

static void frame_init(struct frame * this, uint8_t * data, struct frame_pool_zone * zone)
{
	assert(this != NULL);
	assert(((long)data % MEMPAGE_SIZE) == 0);

	this->type =  0xFFFFFFFF;
	this->zone = zone;
	this->data = data;

	this->borrowed_by_file = NULL;
	this->borrowed_by_line = 0;

	this->dvecs = NULL; //TODO: this ...

	bzero(this->data, FRAME_SIZE);
}

//

static struct frame_pool_zone * frame_pool_zone_new(size_t frame_count, bool extended)
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
	this->_flags.extended = extended;
	this->mmap_size = mmap_size_frames+mmap_size_zone+mmap_size_fill;
	this->next = NULL;

	this->low_frame = &this->frames[0];
	this->high_frame = &this->frames[frame_count-1];

	
	uint8_t * frame_data = p + (mmap_size_zone + mmap_size_fill);
	for(int i=0; i<frame_count; i+=1)
	{
		struct frame * frame = &this->frames[i];
		frame_init(frame, frame_data + (i*FRAME_SIZE), this);
		assert(frame >= this->low_frame);
		assert(frame <= this->high_frame);
	}

	// Prepare stack (S-list) of available (aka all at this time) frames in this zone
	this->available_frames = this->low_frame;
	for(int i=1; i<frame_count; i+=1)
		this->frames[i-1].next_available = &this->frames[i];
	this->high_frame->next_available = NULL;

	L_INFO("Allocated frame pool zone of %zu bytes", this->mmap_size);
	return this;
}

static void frame_pool_zone_del(struct frame_pool_zone * this)
{
	assert(this != NULL);
	munmap(this, this->mmap_size);
}

static struct frame * frame_pool_zone_borrow(struct frame_pool_zone * this, const char * file, unsigned int line)
{
	if (this->available_frames == NULL) return NULL; // Zone has no available frames
	struct frame * frame = this->available_frames;
	this->available_frames = frame->next_available;

	// Reset frame
	frame->type = 0xFFFFFFFF;
	frame->borrowed_by_file = file;
	frame->borrowed_by_line = line;

	return frame;
}

//

bool frame_pool_init(struct frame_pool * this, size_t initial_frame_count)
{
	assert(this != NULL);

	this->zones = frame_pool_zone_new(initial_frame_count, false);
	if (this->zones == NULL) return false;

	return true;
}

void frame_pool_fini(struct frame_pool * this)
{
	assert(this != NULL);

	while(this->zones != NULL)
	{
		struct frame_pool_zone * zone = this->zones;
		this->zones = zone->next;

		frame_pool_zone_del(zone);
	}
}

struct frame * frame_pool_borrow_real(struct frame_pool * this, const char * file, unsigned int line)
{
	assert(this != NULL);

	// Borrow from existing zone
	for (struct frame_pool_zone * zone = this->zones; zone != NULL; zone = zone->next)
	{
		struct frame * frame = frame_pool_zone_borrow(zone, file, line);
		if (frame != NULL) return frame;
	}

	//TODO: There is no frame available, consider creating a new zone and allocate frame from that
	return NULL;
}
