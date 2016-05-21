#ifndef __LIBSCCMN_FRAME_H__
#define __LIBSCCMN_FRAME_H__

struct frame_dvec
{
	struct frame * frame;

	// 0 <= position <= limit <= capacity

	// The position is the index of the next byte to be read or written.
	// The position is never negative and is never greater than its limit.
	size_t position;

	// The limit is the index of the first byte that should not be read or written.
	// The limit is never negative and is never greater than its capacity.
	size_t limit;

	// The capacity is the number of bytes it contains.
	// The capacity is never negative and never changes.
	size_t capacity;
};

static inline void frame_dvec_position_add(struct frame_dvec * this, size_t position_delta)
{
	assert((this->position + position_delta) > 0);
	assert((this->position + position_delta) < this->limit);
	assert((this->position + position_delta) < this->capacity);

	this->position += position_delta;
}

static inline void frame_dvec_set_position(struct frame_dvec * this, size_t position)
{
	this->position = position;
}

static inline void frame_dvec_flip(struct frame_dvec * this)
{
	this->limit = this->position;
	this->position = 0;	
}

bool frame_dvec_sprintf(struct frame_dvec * , const char * format, ...);
bool frame_dvec_vsprintf(struct frame_dvec * , const char * format, va_list ap);
bool frame_dvec_strcat(struct frame_dvec * , const char * text);
bool frame_dvec_cat(struct frame_dvec * , const void * data, size_t data_len);


#define frame_type_FREE    	(0xFFFFFFFF)
#define frame_type_UNKNOWN 	(0xFFFFFFEE)
#define frame_type_RAW 	    (0xFFFFFFDD)


struct frame
{
	struct frame * next; // This allows to chain frames in the list
	struct frame_pool_zone * zone;

	uint64_t type;
	
	//TODO: This is quite temporary
	unsigned int dvec_count;
	struct frame_dvec dvecs[2];

	// Those two are used for tracing and debugging 
	const char * borrowed_by_file;
	unsigned int borrowed_by_line;

	uint8_t * data;
	size_t capacity;
};

size_t frame_total_position(struct frame *);
void frame_format_simple(struct frame *);

#endif //__LIBSCCMN_FRAME_H__
