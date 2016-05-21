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

size_t frame_total_limit(struct frame * this)
{
	assert(this != NULL);

	size_t length = 0;
	for (unsigned int i=0; i<this->dvec_count; i += 1)
	{
		length += this->dvecs[i].limit;
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

bool frame_dvec_vsprintf(struct frame_dvec * this, const char * format, va_list args)
{
	size_t max_size = (this->limit - this->position) - 1;
	int rc = vsnprintf((char *)this->frame->data + this->position, max_size, format, args);

	if (rc < 0) return false;
	if (rc > max_size) return false;
	this->position += rc;	

	return true;
}

bool frame_dvec_sprintf(struct frame_dvec * this, const char * format, ...)
{
	va_list ap;

	va_start(ap, format);
	bool ok = frame_dvec_vsprintf(this, format, ap);
	va_end(ap);

	return ok;
}

bool frame_dvec_cat(struct frame_dvec * this, const void * data, size_t data_len)
{
	if ((this->position + data_len) > this->limit) return false;

	memcpy(this->frame->data + this->position, data, data_len);
	this->position += data_len;

	return true;
}

bool frame_dvec_strcat(struct frame_dvec * this, const char * text)
{
	return frame_dvec_cat(this, text, strlen(text));
}
