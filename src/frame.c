#include "all.h"

void _frame_init(struct frame * this, uint8_t * data, size_t capacity, struct frame_pool_zone * zone)
{
	assert(this != NULL);
	assert(((long)data % MEMPAGE_SIZE) == 0);

	this->type = frame_type_FREE;
	this->zone = zone;
	this->data = data;
	this->capacity = capacity;

	this->borrowed_by_file = NULL;
	this->borrowed_by_line = 0;

	this->dvec_count = 0;

	bzero(this->data, FRAME_SIZE);
}

size_t frame_total_position(struct frame * this)
{
	assert(this != NULL);

	size_t length = 0;
	for (unsigned int i=0; i<this->dvec_count; i += 1)
	{
		length += this->dvecs[i].position;
	}

	return length;
}

void frame_format_simple(struct frame * this)
{
	assert(this != NULL);

	this->dvec_count = 1;
	this->dvecs[0].frame = this;
	this->dvecs[0].position = 0;
	this->dvecs[0].limit = this->capacity;
	this->dvecs[0].capacity = this->capacity;
}