#ifndef __LIBSCCMN_FRAME_H__
#define __LIBSCCMN_FRAME_H__

/****
Frame:

+-------------------------------+
|            data space         |
+-------------------------------+ 
|            free space         |
+-------------------------------+
|        dvec[-dvec_limit-1]    |
+-------------------------------+
|        dvec[-dvec_limit-2]    |
+-------------------------------+
|             dvec[0]           |
+-------------------------------+ <----- capacity 

****/

struct frame_dvec
{
	struct frame * frame;

	// 0 (+ offset) <= position <= limit <= capacity

	// 
	// The offset is never negative and is inside a frame
	size_t offset;

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
	assert((this->position + position_delta) >= 0);
	assert((this->position + position_delta) <= this->limit);
	assert((this->position + position_delta) <= this->capacity);

	this->position += position_delta;
}

static inline void frame_dvec_position_set(struct frame_dvec * this, size_t position)
{
	assert(this->position <= this->limit);
	assert(this->position <= this->capacity);
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


#define frame_type_FREE    	   (0xFFFFFFFF)
#define frame_type_STREAM_END  (0xFFFFFFFE)
#define frame_type_RAW_DATA	   (0xFFFFFFFD)

struct frame
{
	struct frame * next; // This allows to chain frames in the list
	struct frame_pool_zone * zone;

	uint64_t type;
	
	unsigned int dvec_position;
	unsigned int dvec_limit;

	// Those two are used for tracing and debugging 
	const char * borrowed_by_file;
	unsigned int borrowed_by_line;

	uint8_t * data;
	size_t capacity;
};

size_t frame_total_start_to_position(struct frame *);
size_t frame_total_position_to_limit(struct frame *);

void frame_format_empty(struct frame *);
void frame_format_simple(struct frame *);


static inline void * frame_dvec_ptr(struct frame_dvec * this)
{
	assert(this != NULL);
	assert(this->frame != NULL);
	return this->frame->data + this->offset + this->position;
}

static inline size_t frame_dvec_len(struct frame_dvec * this)
{
	assert(this != NULL);
	return this->limit - this->position;
}


//This is diagnostics function
void frame_print(struct frame *);

static inline void frame_flip(struct frame * this)
{
	assert(this != NULL);
	struct frame_dvec * dvec = (struct frame_dvec *)(this->data + this->capacity);
	for (int i=-1; i>(-1-this->dvec_limit); i -= 1)
	{
		frame_dvec_flip(&dvec[i]);
	}

	// Reset also to the original dvec position
	this->dvec_position = 0;
}

static inline void frame_set_type(struct frame * this, uint64_t type)
{
	assert(this != NULL);
	this->type = type;
}

///

struct frame_dvec * frame_add_dvec(struct frame * this, size_t offset, size_t capacity);

static inline bool frame_next_dvec(struct frame * this)
{
	assert(this != NULL);
	if (this->dvec_position == this->dvec_limit) return false;
	assert(this->dvec_position < this->dvec_limit);
	this->dvec_position += 1;
	return (this->dvec_position < this->dvec_limit);
}

static inline struct frame_dvec * frame_current_dvec(struct frame * this)
{
	assert(this != NULL);
	if (this->dvec_position == this->dvec_limit) return NULL;
	assert(this->dvec_position < this->dvec_limit);

	struct frame_dvec * dvec = (struct frame_dvec *)(this->data + this->capacity);
	dvec -= (1 + this->dvec_position);
	return dvec;
}

static inline size_t frame_currect_dvec_size(struct frame * this)
{
	assert(this != NULL);
	struct frame_dvec * dvec = frame_current_dvec(this);
	return dvec->limit - dvec->position;
}

#endif //__LIBSCCMN_FRAME_H__
