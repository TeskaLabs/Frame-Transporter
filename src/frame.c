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

	this->dvec_position = 0;
	this->dvec_limit = 0;

	bzero(this->data, FRAME_SIZE);
}

size_t frame_total_start_to_position(struct frame * this)
{
	assert(this != NULL);

	size_t length = 0;
	struct frame_dvec * dvecs = (struct frame_dvec *)(this->data + this->capacity);
	for (int i=-1; i>(-1-this->dvec_limit); i -= 1)
	{
		length += dvecs[i].position;
	}

	return length;
}

size_t frame_total_position_to_limit(struct frame * this)
{
	assert(this != NULL);

	size_t length = 0;
	struct frame_dvec * dvecs = (struct frame_dvec *)(this->data + this->capacity);
	for (int i=-1; i>(-1-this->dvec_limit); i -= 1)
	{
		length += dvecs[i].limit - dvecs[i].position;
	}

	return length;
}

struct frame_dvec * frame_add_dvec(struct frame * this, size_t offset, size_t capacity)
{
	assert(this != NULL);	
	assert(offset >= 0);
	assert(capacity >= 0);

	struct frame_dvec * dvec = (struct frame_dvec *)(this->data + this->capacity);
	dvec -= this->dvec_limit + 1;

	//Test if there is enough space in the frame
	if (((uint8_t *)dvec - this->data) < (offset + capacity))
	{
		L_ERROR("Cannot accomodate that dvec in the current frame.");
		return NULL;
	}


	this->dvec_limit += 1;

	assert((uint8_t *)dvec  > this->data);
	assert((void *)dvec + this->dvec_limit * sizeof(struct frame_dvec) == (this->data + this->capacity));

	dvec->frame = this;
	dvec->offset = offset;
	dvec->position = 0;
	dvec->limit = dvec->capacity = capacity;

	return dvec;
}

void frame_format_empty(struct frame * this)
{
	this->dvec_limit = 0;
	this->dvec_position = 0;	
}

void frame_format_simple(struct frame * this)
{
	frame_format_empty(this);
	frame_add_dvec(this, 0, this->capacity - sizeof(struct frame_dvec));
}


bool frame_dvec_vsprintf(struct frame_dvec * this, const char * format, va_list args)
{
	size_t max_size = (this->limit - this->position) - 1;
	int rc = vsnprintf((char *)this->frame->data + this->offset + this->position, max_size, format, args);

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

	memcpy(this->frame->data + this->offset + this->position, data, data_len);
	this->position += data_len;

	return true;
}

bool frame_dvec_strcat(struct frame_dvec * this, const char * text)
{
	return frame_dvec_cat(this, text, strlen(text));
}

void frame_print(struct frame * this)
{
	struct frame_dvec * dvecs = (struct frame_dvec *)(this->data + this->capacity);
	for (int i=-1; i>(-1-this->dvec_limit); i -= 1)
		write(STDOUT_FILENO, dvecs[i].frame->data + dvecs[i].offset, dvecs[i].limit);
}
