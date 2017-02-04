#include "../_ft_internal.h"

void _ft_frame_init(struct ft_frame * this, uint8_t * data, size_t capacity, struct ft_poolzone * zone)
{
	assert(this != NULL);
	assert(((long)data % MEMPAGE_SIZE) == 0);

	this->type = FT_FRAME_TYPE_FREE;
	this->zone = zone;
	this->data = data;
	this->capacity = capacity;

	this->addrlen = 0;
	this->addr.ss_family = AF_UNSPEC;

	this->borrowed_by_file = NULL;
	this->borrowed_by_line = 0;

	this->vec_position = 0;
	this->vec_limit = 0;

	this->flags.msg_control = false;

	bzero(this->data, FRAME_SIZE);
}

size_t ft_frame_pos(struct ft_frame * this)
{
	assert(this != NULL);

	size_t length = 0;
	struct ft_vec * vecs = (struct ft_vec *)(this->data + this->capacity);
	for (int i=-1; i>(-1-this->vec_limit); i -= 1)
	{
		length += vecs[i].position;
	}

	return length;
}

size_t ft_frame_len(struct ft_frame * this)
{
	assert(this != NULL);

	size_t length = 0;
	struct ft_vec * vecs = (struct ft_vec *)(this->data + this->capacity);
	for (int i=-1; i>(-1-this->vec_limit); i -= 1)
	{
		length += vecs[i].limit - vecs[i].position;
	}

	return length;
}

struct ft_vec * ft_frame_create_vec(struct ft_frame * this, size_t offset, size_t capacity)
{
	assert(this != NULL);	
	assert(offset >= 0);
	assert(capacity >= 0);

	struct ft_vec * vec = (struct ft_vec *)(this->data + this->capacity);
	vec -= this->vec_limit + 1;

	//Test if there is enough space in the frame
	if (((uint8_t *)vec - this->data) < (offset + capacity))
	{
		FT_ERROR("Cannot accomodate that vec in the current frame");
		return NULL;
	}

	this->vec_limit += 1;

	assert((uint8_t *)vec  > this->data);
	assert((void *)vec + this->vec_limit * sizeof(struct ft_vec) == (this->data + this->capacity));

	vec->frame = this;
	vec->offset = offset;
	vec->position = 0;
	vec->limit = vec->capacity = capacity;

	return vec;
}

struct ft_vec * ft_frame_append_vec(struct ft_frame * this, size_t capacity)
{
	assert(this != NULL);	
	assert(capacity >= 0);

	struct ft_vec * vec = (struct ft_vec *)(this->data + this->capacity);
	vec -= this->vec_limit + 1;

	size_t offset = 0;

	if (this->vec_limit > 0)
	{
		struct ft_vec * prev_vec = (struct ft_vec *)(this->data + this->capacity);
		prev_vec -= this->vec_limit;

		if (prev_vec->frame != this)
		{
			FT_ERROR("Previous vector points to different frame (%p!=%p), it is not supported", prev_vec->frame, this);
			return NULL;
		}

		offset = prev_vec->offset + prev_vec->capacity;
	}
	assert(offset >= 0);

	//Test if there is enough space in the frame
	if (((uint8_t *)vec - this->data) < (offset + capacity))
	{
		// Silent error, because this can happen during the log rotation 
		return NULL;
	}

	this->vec_limit += 1;

	assert((uint8_t *)vec  > this->data);
	assert((void *)vec + this->vec_limit * sizeof(struct ft_vec) == (this->data + this->capacity));

	vec->frame = this;
	vec->offset = offset;
	vec->position = 0;
	vec->limit = vec->capacity = capacity;

	return vec;
}

struct ft_vec * ft_frame_append_max_vec(struct ft_frame * this)
{
	struct ft_vec * vec = ft_frame_append_vec(this, 0);
	vec->limit = vec->capacity = ((uint8_t *)vec - this->data) - vec->offset;
	return vec;
}

void ft_frame_format_empty(struct ft_frame * this)
{
	assert(this != NULL);

	this->vec_limit = 0;
	this->vec_position = 0;	
}

void ft_frame_format_simple(struct ft_frame * this)
{
	ft_frame_format_empty(this);
	ft_frame_create_vec(this, 0, this->capacity - sizeof(struct ft_vec));
}


void ft_frame_debug(struct ft_frame * this)
{
	FT_DEBUG("Frame structure: VP:%u/VL:%u d:%p", this->vec_position, this->vec_limit, this->data);
	
	struct ft_vec * vec = (struct ft_vec *)(this->data + this->capacity);
	for (int i=-1; i>(-1-this->vec_limit); i -= 1)
		FT_DEBUG(" - vec #%d o:%u p:%u l:%u c:%u d:%p%s",
			-i,
			vec[i].offset, vec[i].position, vec[i].limit, vec[i].capacity,
			vec[i].frame->data, vec[i].frame->data == this->data ? "" : "!"
		);
}

/// ft_vec methods follows ...

bool ft_vec_vsprintf(struct ft_vec * this, const char * format, va_list args)
{
	assert(this != NULL);

	size_t max_size = (this->limit - this->position) - 1;
	int rc = vsnprintf((char *)ft_vec_ptr(this), max_size, format, args);

	if (rc < 0) return false;
	if (rc > max_size) return false;
	this->position += rc;	

	return true;
}

bool ft_vec_sprintf(struct ft_vec * this, const char * format, ...)
{
	assert(this != NULL);

	va_list ap;

	va_start(ap, format);
	bool ok = ft_vec_vsprintf(this, format, ap);
	va_end(ap);

	return ok;
}

bool ft_vec_cat(struct ft_vec * this, const void * data, size_t data_len)
{
	assert(this != NULL);

	if ((this->position + data_len) > this->limit) return false;

	memcpy(ft_vec_ptr(this), data, data_len);
	this->position += data_len;

	return true;
}

bool ft_vec_strcat(struct ft_vec * this, const char * text)
{
	assert(this != NULL);

	return ft_vec_cat(this, text, strlen(text));
}

// File related functions ...

bool ft_frame_fread(struct ft_frame * this, FILE * f)
{
	assert(this != NULL);

	ft_frame_format_simple(this);

	fseek(f, 0L, SEEK_END);
	long sz = ftell(f);
	if (sz < 0)
	{
		FT_ERROR_ERRNO_P(errno, "ftell");
		return false;
	}
	size_t ssz = sz;
	rewind(f);

	struct ft_vec * vec = ft_frame_get_vec(this);
	assert(vec != NULL);
	void * p = ft_vec_ptr(vec);
	assert(p != NULL);

	if (sz > vec->limit)
	{
		FT_WARN("File doesn't fit into a frame (%u < %ld), truncating", vec->limit, sz);
		sz = vec->limit;
	}

	size_t rc = fread(p, ssz, 1, f);
	if (rc != 1)
	{
		FT_ERROR_ERRNO(errno, "fread");
	}

	if (rc != 1)
		return false;

	ft_vec_advance(vec, sz);

	return true;
}


bool ft_frame_fwrite(struct ft_frame * this, FILE * f)
{
	assert(this != NULL);

	struct ft_vec * vec = (struct ft_vec *)(this->data + this->capacity);
	for (int i=-1; i>(-1-this->vec_limit); i -= 1)
	{
		size_t rc = fwrite(vec[i].frame->data + vec[i].offset, vec[i].limit, 1, f);
		if (rc != 1)
		{
			FT_ERROR_ERRNO(errno, "fwrite");
			return false;
		}
	}
	return true;
}

bool ft_frame_save(struct ft_frame * this, const char * filename)
{
	assert(this != NULL);
	bool ok;

	FILE * f = fopen(filename, "wb");
	if (f == NULL)
	{
		FT_WARN_ERRNO(errno, "Error when opening file '%s'", filename);
		return false;
	}

	ok = ft_frame_fwrite(this, f);

	fclose(f);

	return ok;
}
