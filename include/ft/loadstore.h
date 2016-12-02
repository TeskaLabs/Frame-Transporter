#ifndef FT_LOADSTORE_H_
#define FT_LOADSTORE_H_

// Contains fast load/store functions that are usable e.g. for a parsing of the protocol

static inline uint8_t * ft_skip_bytes(uint8_t * cursor, size_t bytes)
{
	return cursor+bytes;
}

static inline uint8_t * ft_skip_u8(uint8_t * cursor)
{
	return cursor+1;
}

static inline uint8_t * ft_skip_u16(uint8_t * cursor)
{
	return cursor+2;
}

static inline uint8_t * ft_skip_u24(uint8_t * cursor)
{
	return cursor+3;
}

static inline uint8_t * ft_skip_u32(uint8_t * cursor)
{
	return cursor+4;
}

static inline uint8_t * ft_skip_u64(uint8_t * cursor)
{
	return cursor+8;
}


static inline uint8_t * ft_load_u8(uint8_t * cursor, uint8_t * value)
{
	*value = *cursor;
	return ft_skip_u8(cursor);
}

static inline uint8_t * ft_load_u16(uint8_t * cursor, uint16_t * value)
{
	*value = cursor[1] | (cursor[0] << 8);
	return ft_skip_u16(cursor);
}

static inline uint8_t * ft_load_u24(uint8_t * cursor, uint32_t * value)
{
	*value = cursor[2] | (cursor[1] << 8) | (cursor[0] << 16);
	return ft_skip_u24(cursor);
}

static inline uint8_t * ft_load_u32(uint8_t * cursor, uint32_t * value)
{
	*value = cursor[3] | (cursor[2] << 8) | (cursor[1] << 16) | (cursor[0] << 24);
	return ft_skip_u32(cursor);
}

static inline uint8_t * ft_load_f64(uint8_t * cursor, double * value)
{
	assert(sizeof(*value) == 8);

	uint8_t bval[8];
	bval[7] = cursor[0];
	bval[6] = cursor[1];
	bval[5] = cursor[2];
	bval[4] = cursor[3];
	bval[3] = cursor[4];
	bval[2] = cursor[5];
	bval[1] = cursor[6];
	bval[0] = cursor[7];

	memcpy(value, bval, 8);
	return ft_skip_u64(cursor);
}

static inline uint8_t * ft_load_bytes(uint8_t * cursor, void * trg, size_t n)
{
	memcpy(trg, cursor, n);
	return cursor + n;
}


static inline uint8_t * ft_store_u8(uint8_t * cursor, uint8_t value)
{
	*cursor = value;
	return ft_skip_u8(cursor);
}

static inline uint8_t * ft_store_u16(uint8_t * cursor, uint16_t value)
{
	cursor[0] = 0xFF & (value >> 8);
	cursor[1] = 0xFF & value;
	return ft_skip_u16(cursor);
}

static inline uint8_t * ft_store_u24(uint8_t * cursor, uint32_t value)
{
	cursor[0] = 0xFF & (value >> 16);
	cursor[1] = 0xFF & (value >> 8);
	cursor[2] = 0xFF & value;
	return ft_skip_u24(cursor);
}

static inline uint8_t * ft_store_u32(uint8_t * cursor, uint32_t value)
{
	cursor[0] = 0xFF & (value >> 24);
	cursor[1] = 0xFF & (value >> 16);
	cursor[2] = 0xFF & (value >> 8);
	cursor[3] = 0xFF & value;
	return ft_skip_u32(cursor);
}

static inline uint8_t * ft_store_f64(uint8_t * cursor, double value)
{
	assert(sizeof(value) == 8);

	uint8_t bval[8];
	memcpy(bval, &value, sizeof(value));

	cursor[0] = bval[7];
	cursor[1] = bval[6];
	cursor[2] = bval[5];
	cursor[3] = bval[4];
	cursor[4] = bval[3];
	cursor[5] = bval[2];
	cursor[6] = bval[1];
	cursor[7] = bval[0];

	return ft_skip_u64(cursor);
}

static inline uint8_t * ft_store_bytes(uint8_t * cursor, const void * src, size_t n)
{
	memcpy(cursor, src, n);
	return cursor + n;
}

#endif // FT_LOADSTORE_H_
