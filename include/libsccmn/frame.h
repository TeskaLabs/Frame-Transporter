#ifndef __LIBSCCMN_FRAME_H__
#define __LIBSCCMN_FRAME_H__

struct frame_dvec
{
	uint8_t * base;

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

enum frame_types
{
	frame_type_FREE    = 0xFFFFFFFF,
	frame_type_UNKNOWN = 0xFFFFFFEE
};

struct frame
{
	struct frame * next; // This allows to chain frames in the list
	struct frame_pool_zone * zone;

	enum frame_types type;
	struct frame_dvec * dvecs;

	// Those two are used for tracing and debugging 
	const char * borrowed_by_file;
	unsigned int borrowed_by_line;

	uint8_t * data;
};

size_t frame_capacity(struct frame *);

#endif //__LIBSCCMN_FRAME_H__
