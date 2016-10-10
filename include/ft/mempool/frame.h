#ifndef FT_MEMPOOL_FRAME_H_
#define FT_MEMPOOL_FRAME_H_

enum ft_frame_type
{
	FT_FRAME_TYPE_FREE           = 0xFFFFFFFF,
	FT_FRAME_TYPE_STREAM_END     = 0xFFFFFFFE,
    FT_FRAME_TYPE_RAW_DATA       = 0xFFFFFFFD,
};

struct ft_frame
{
	struct ft_frame * next; // This allows to chain frames in the list
	struct ft_poolzone * zone;

	enum ft_frame_type type;
	
	unsigned int vec_position;
	unsigned int vec_limit;

	// Used to store address of inbound/outbound socket
	struct sockaddr_storage addr;
	socklen_t addrlen;

	// Those two are used for tracing and debugging 
	const char * borrowed_by_file;
	unsigned int borrowed_by_line;

	uint8_t * data;
	size_t capacity;
};

size_t ft_frame_pos(struct ft_frame *);
size_t ft_frame_len(struct ft_frame *);

void ft_frame_format_empty(struct ft_frame *);
void ft_frame_format_simple(struct ft_frame *);

bool ft_frame_fwrite(struct ft_frame *, FILE * f);
bool ft_frame_fread(struct ft_frame * , FILE * f);

void ft_frame_debug(struct ft_frame *);

static inline void ft_frame_flip(struct ft_frame * this)
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

static inline void ft_frame_set_type(struct ft_frame * this, enum ft_frame_type type)
{
	assert(this != NULL);
	this->type = type;
}

///

struct ft_vec * ft_frame_create_vec(struct ft_frame * this, size_t offset, size_t capacity);

static inline struct ft_vec * ft_frame_next_vec(struct ft_frame * this)
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

static inline struct ft_vec * ft_frame_prev_vec(struct ft_frame * this)
{
	assert(this != NULL);
	if (this->vec_position == 0) return NULL;
	this->vec_position -= 1;

	struct ft_vec * vec = (struct ft_vec *)(this->data + this->capacity);
	vec -= (1 + this->vec_position);
	return vec;
}


static inline struct ft_vec * ft_frame_get_vec(struct ft_frame * this)
{
	assert(this != NULL);
	if (this->vec_position == this->vec_limit) return NULL;
	assert(this->vec_position < this->vec_limit);

	struct ft_vec * vec = (struct ft_vec *)(this->data + this->capacity);
	vec -= (1 + this->vec_position);
	return vec;
}


static inline struct ft_vec * ft_frame_get_vec_at(struct ft_frame * this, size_t position)
{
	assert(this != NULL);
	if (position >= this->vec_limit) return NULL;

	struct ft_vec * vec = (struct ft_vec *)(this->data + this->capacity);
	vec -= (1 + position);
	return vec;
}

static inline void ft_frame_reset_vec(struct ft_frame * this)
{
	assert(this != NULL);
	this->vec_position = 0;
}

// Following function requires access to frame object internals
// and for this reason it is here and not with ft_vec object
static inline void * ft_vec_ptr(struct ft_vec * this)
{
	assert(this != NULL);
	assert(this->frame != NULL);
	return this->frame->data + this->offset + this->position;
}

// Following function requires access to frame object internals
// and for this reason it is here and not with ft_vec object
static inline void * ft_vec_begin_ptr(struct ft_vec * this)
{
	assert(this != NULL);
	assert(this->frame != NULL);
	return this->frame->data + this->offset;
}


#endif // FT_MEMPOOL_FRAME_H_
