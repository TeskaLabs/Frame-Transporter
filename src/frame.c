#include "all.h"

void _frame_init(struct frame * this, uint8_t * data, struct frame_pool_zone * zone)
{
	assert(this != NULL);
	assert(((long)data % MEMPAGE_SIZE) == 0);

	this->type = frame_type_FREE;
	this->zone = zone;
	this->data = data;

	this->borrowed_by_file = NULL;
	this->borrowed_by_line = 0;

	this->dvecs = NULL; //TODO: this ...

	bzero(this->data, FRAME_SIZE);
}

size_t frame_capacity(struct frame * this)
{
	return 0;
}
