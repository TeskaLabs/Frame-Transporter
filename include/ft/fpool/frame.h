#ifndef FT_FRAME_H_
#define FT_FRAME_H_

enum ft_frame_type
{
	FT_FRAME_TYPE_FREE           = 0xFFFFFFFF,
	FT_FRAME_TYPE_STREAM_END     = 0xFFFFFFFE,
    FT_FRAME_TYPE_RAW_DATA       = 0xFFFFFFFD,
};

struct frame
{
	struct frame * next; // This allows to chain frames in the list
	struct frame_pool_zone * zone;

	enum ft_frame_type type;
	
	unsigned int vec_position;
	unsigned int vec_limit;

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

//This is diagnostics function
void frame_print(struct frame *);

static inline void frame_flip(struct frame * this)
{
	assert(this != NULL);
	struct ft_vec * vec = (struct ft_vec *)(this->data + this->capacity);
	for (int i=-1; i>(-1-this->vec_limit); i -= 1)
	{
		ft_vec_flip(&vec[i]);
	}

	// Reset also to the original vec position
	this->vec_position = 0;
}

static inline void frame_set_type(struct frame * this, enum ft_frame_type type)
{
	assert(this != NULL);
	this->type = type;
}

///

struct ft_vec * frame_add_dvec(struct frame * this, size_t offset, size_t capacity);

static inline struct ft_vec * frame_next_dvec(struct frame * this)
{
	assert(this != NULL);
	if (this->vec_position == this->vec_limit) return NULL;
	assert(this->vec_position < this->vec_limit);
	this->vec_position += 1;
	if (this->vec_position == this->vec_limit) return NULL;

	struct ft_vec * vec = (struct ft_vec *)(this->data + this->capacity);
	vec -= (1 + this->vec_position);
	return vec;
}

static inline struct ft_vec * frame_current_dvec(struct frame * this)
{
	assert(this != NULL);
	if (this->vec_position == this->vec_limit) return NULL;
	assert(this->vec_position < this->vec_limit);

	struct ft_vec * vec = (struct ft_vec *)(this->data + this->capacity);
	vec -= (1 + this->vec_position);
	return vec;
}

static inline size_t frame_currect_dvec_size(struct frame * this)
{
	assert(this != NULL);
	struct ft_vec * vec = frame_current_dvec(this);
	return vec->limit - vec->position;
}

// Following function requires access to frame object internals
// and for this reason it is here and not with ft_vec object
static inline void * ft_vec_ptr(struct ft_vec * this)
{
	assert(this != NULL);
	assert(this->frame != NULL);
	return this->frame->data + this->offset + this->position;
}

#endif // FT_FRAME_H_
