#include "../_ft_internal.h"
#include <arpa/inet.h>

static struct ft_iphashmap_entry ** ft_iphashmap_alloc(size_t size);

///

bool ft_iphashmap_init(struct ft_iphashmap * this, int initial_bucket_size)
{
	assert(this != NULL);

	this->count = 0;
	this->bucket_size = initial_bucket_size;
	this->buckets = NULL;

	return true;
}


void ft_iphashmap_fini(struct ft_iphashmap * this)
{
	assert(this != NULL);
	if (this->buckets != NULL)
	{
		free(this->buckets);
		this->buckets = NULL;
	}
}


void ft_iphashmap_clear(struct ft_iphashmap * this)
{
	assert(this != NULL);

	this->count = 0;
	if (this->buckets != NULL)
	{
		free(this->buckets);
		this->buckets = NULL;
	}
}

/// Hash functions

static inline unsigned int ft_iphashmap_hash_ip4(struct in_addr * ip4)
{
	unsigned int x1 = ip4->s_addr & 0xFFFF;
	unsigned int x2 = (ip4->s_addr >> 16) & 0xFFFF;
	return AF_INET ^ x1 ^ x2;
}


static inline unsigned int ft_iphashmap_hash_ip6(struct in6_addr * ip6)
{
	unsigned int x1 = (ip6->s6_addr[0] << 8) | ip6->s6_addr[1];
	unsigned int x2 = (ip6->s6_addr[2] << 8) | ip6->s6_addr[3];
	unsigned int x3 = (ip6->s6_addr[4] << 8) | ip6->s6_addr[5];
	unsigned int x4 = (ip6->s6_addr[6] << 8) | ip6->s6_addr[7];
	unsigned int x5 = (ip6->s6_addr[8] << 8) | ip6->s6_addr[9];
	unsigned int x6 = (ip6->s6_addr[10] << 8) | ip6->s6_addr[11];
	unsigned int x7 = (ip6->s6_addr[12] << 8) | ip6->s6_addr[13];
	unsigned int x8 = (ip6->s6_addr[14] << 8) | ip6->s6_addr[15];
	return AF_INET6 ^ x1 ^ x2 ^ x3 ^ x4 ^ x5 ^ x6 ^ x7 ^ x8;
}

/// Generics

static struct ft_iphashmap_entry * ft_iphashmap_add(struct ft_iphashmap * this, int family, union ft_iphashmap_addr addr)
{
	assert(this != NULL);

	unsigned int hash = 0;
	switch (family)
	{
		case AF_INET:
			hash = ft_iphashmap_hash_ip4(&addr.ip4);
			break;

		case AF_INET6:
			hash = ft_iphashmap_hash_ip6(&addr.ip6);
			break;

		default:
			FT_ERROR("Unimplemented family (ft_iphashmap_add): %d", family);
			return NULL;
	}

	if (this->buckets == NULL)
	{
		// Lazy allocation
		this->buckets = ft_iphashmap_alloc(this->bucket_size);
		if (this->buckets == NULL) return false;
	}

	unsigned int index = hash % this->bucket_size;

	struct ft_iphashmap_entry ** bucket_ptr = &this->buckets[index];
	for(;*bucket_ptr != NULL; bucket_ptr = &(*bucket_ptr)->next)
	{
		struct ft_iphashmap_entry * bucket = * bucket_ptr;
		if (bucket->family != family) continue;
		switch (family)
		{
			case AF_INET:
				if (bucket->addr.ip4.s_addr != addr.ip4.s_addr) continue;
				break;

			case AF_INET6:
				if (memcmp(bucket->addr.ip6.s6_addr, addr.ip6.s6_addr, sizeof(addr.ip6.s6_addr)) != 0) continue;
				break;

			default:
				continue;
		}

		// We found a collision
		return bucket;
	}

	assert(bucket_ptr != NULL);
	assert(*bucket_ptr == NULL);
	
	struct ft_iphashmap_entry * entry = malloc(sizeof(struct ft_iphashmap_entry));
	if (entry == NULL)
	{
		FT_ERROR_ERRNO(errno, "malloc(sizeof(struct ft_iphashmap_entry)=%zu)", sizeof(struct ft_iphashmap_entry));
		return NULL;
	}

	entry->family = family;
	if (entry->family == AF_INET)
	{
		entry->addr.ip4 = addr.ip4;
	}
	else if (entry->family == AF_INET6)
	{
		entry->addr.ip6 = addr.ip6;
	}

	entry->next = NULL;
	*bucket_ptr = entry;

	this->count += 1;

	return entry;
}

static struct ft_iphashmap_entry * ft_iphashmap_get(struct ft_iphashmap * this, unsigned int hash, int family, union ft_iphashmap_addr addr)
{
	assert(this != NULL);
	if (this->buckets == NULL) return NULL; // Hash map is empty

	unsigned int index = hash % this->bucket_size;
	for(struct ft_iphashmap_entry * bucket = this->buckets[index]; bucket != NULL; bucket = bucket->next)
	{
		if (bucket->family != family) continue;
		switch (family)
		{

			case AF_INET:
				if (bucket->addr.ip4.s_addr != addr.ip4.s_addr) continue;
				break;

			case AF_INET6:
				if (memcmp(bucket->addr.ip6.s6_addr, addr.ip6.s6_addr, sizeof(addr.ip6.s6_addr)) != 0) continue;
				break;

			default:
				continue;
		}
		return bucket;
	}

	return NULL;
}

static bool ft_iphashmap_pop(struct ft_iphashmap * this, unsigned int hash, int family, union ft_iphashmap_addr addr, void ** data)
{
	assert(this != NULL);
	if (this->buckets == NULL) return false; // Hash map is empty

	unsigned int index = hash % this->bucket_size;
	for(struct ft_iphashmap_entry ** bucket_ptr = &this->buckets[index]; *bucket_ptr != NULL; bucket_ptr = &(*bucket_ptr)->next)
	{
		struct ft_iphashmap_entry * bucket = * bucket_ptr;
		if (bucket->family != family) continue;
		switch (family)
		{
			case AF_INET:
				if (bucket->addr.ip4.s_addr != addr.ip4.s_addr) continue;
				break;

			case AF_INET6:
				if (memcmp(bucket->addr.ip6.s6_addr, addr.ip6.s6_addr, sizeof(addr.ip6.s6_addr)) != 0) continue;
				break;

			default:
				continue;
		}
		
		// We found the bucket
		*bucket_ptr = bucket->next;
		if (data != NULL) *data = bucket->data;
		free(bucket);

		assert(this->count > 0);
		this->count -= 1;

		if (this->count == 0)
		{
			free(this->buckets);
			this->buckets = NULL;
		}

		return true;
	}

	return false;
}

/// IPv4

struct ft_iphashmap_entry * ft_iphashmap_add_ip4(struct ft_iphashmap * this, struct in_addr addr)
{
	union ft_iphashmap_addr uaddr = { .ip4 = addr };
	return ft_iphashmap_add(this, AF_INET, uaddr);
}

struct ft_iphashmap_entry * ft_iphashmap_get_ip4(struct ft_iphashmap * this, struct in_addr addr)
{
	union ft_iphashmap_addr uaddr = { .ip4 = addr };
	return ft_iphashmap_get(this, ft_iphashmap_hash_ip4(&addr), AF_INET, uaddr);
}

bool ft_iphashmap_pop_ip4(struct ft_iphashmap * this, struct in_addr addr, void ** data)
{
	union ft_iphashmap_addr uaddr = { .ip4 = addr };
	return ft_iphashmap_pop(this, ft_iphashmap_hash_ip4(&addr), AF_INET, uaddr, data);
}

/// IPv6

struct ft_iphashmap_entry * ft_iphashmap_add_ip6(struct ft_iphashmap * this, struct in6_addr addr)
{
	union ft_iphashmap_addr uaddr = { .ip6 = addr };
	return ft_iphashmap_add(this, AF_INET6, uaddr);

}

struct ft_iphashmap_entry * ft_iphashmap_get_ip6(struct ft_iphashmap * this, struct in6_addr addr)
{
	union ft_iphashmap_addr uaddr = { .ip6 = addr };
	return ft_iphashmap_get(this, ft_iphashmap_hash_ip6(&addr), AF_INET6, uaddr);
}

bool ft_iphashmap_pop_ip6(struct ft_iphashmap * this, struct in6_addr addr, void ** data)
{
	union ft_iphashmap_addr uaddr = { .ip6 = addr };
	return ft_iphashmap_pop(this, ft_iphashmap_hash_ip6(&addr), AF_INET6, uaddr, data);
}

///

struct ft_iphashmap_entry * ft_iphashmap_add_p(struct ft_iphashmap * this, int family, const void * src)
{
	int rc;
	assert(this != NULL);

retry:
	switch (family)
	{
		case AF_INET:
		{
			struct in_addr addr;
			rc = inet_pton(AF_INET, src, &addr);
			if (rc != 1) return NULL;
			return ft_iphashmap_add_ip4(this, addr);
		}

		case AF_INET6:
		{
			struct in6_addr addr;
			rc = inet_pton(AF_INET6, src, &addr);
			if (rc != 1) return NULL;
			return ft_iphashmap_add_ip6(this, addr);
		}

		case AF_UNSPEC:
		{
			if (strchr(src, ':') != NULL)
			{
				family = AF_INET6;
				goto retry;
			}

			if (strchr(src, '.') != NULL)
			{
				family = AF_INET;
				goto retry;
			}

			FT_WARN("Cannot autodetect address family of '%s'", src);
			return NULL;

		}
	}

	FT_WARN("Unsupported family: %d", family);
	return NULL;
}


bool ft_iphashmap_pop_p(struct ft_iphashmap * this, int family, const void * src, void ** data)
{
	int rc;
	assert(this != NULL);

retry:
	switch (family)
	{
		case AF_INET:
		{
			struct in_addr addr;
			rc = inet_pton(AF_INET, src, &addr);
			if (rc != 1) return false;
			return ft_iphashmap_pop_ip4(this, addr, data);
		}

		case AF_INET6:
		{
			struct in6_addr addr;
			rc = inet_pton(AF_INET6, src, &addr);
			if (rc != 1) return false;
			return ft_iphashmap_pop_ip6(this, addr, data);
		}

		case AF_UNSPEC:
		{
			if (strchr(src, ':') != NULL)
			{
				family = AF_INET6;
				goto retry;
			}

			if (strchr(src, '.') != NULL)
			{
				family = AF_INET;
				goto retry;
			}

			FT_WARN("Cannot autodetect address family of '%s'", src);
			return false;

		}
	}

	FT_WARN("Unsupported family: %d", family);
	return false;
}


struct ft_iphashmap_entry * ft_iphashmap_get_sa(struct ft_iphashmap * this, const struct sockaddr * addr, socklen_t addrlen)
{
	if (addr == NULL) return NULL;

	switch (addr->sa_family)
	{
		case AF_INET:
		{
			const struct sockaddr_in * addr_in = (const struct sockaddr_in *)addr;
			return ft_iphashmap_get_ip4(this, addr_in->sin_addr);
		}

		case AF_INET6:
		{
			const struct sockaddr_in6 * addr_in = (const struct sockaddr_in6 *)addr;
			return ft_iphashmap_get_ip6(this, addr_in->sin6_addr);
		}

		default:
			FT_WARN("Unsupported family: %d", addr->sa_family);
			return NULL;

	}
}

///

static struct ft_iphashmap_entry ** ft_iphashmap_alloc(size_t size)
{
	struct ft_iphashmap_entry ** buckets = calloc(size, sizeof(struct ft_iphashmap_entry *));
	if (buckets == NULL) return NULL;
	for (int i = 0; i < size; i++)
		buckets[i] = NULL;

	return buckets;
}

///

static inline char *ft_iphashmap_trim_whitespace(char *str)
{
	char *end;

	// Trim leading space
	while(isspace((unsigned char)*str)) str++;

	if(*str == 0)  // All spaces?
		return str;

	// Trim trailing space
	end = str + strlen(str) - 1;
	while(end > str && isspace((unsigned char)*end)) end--;

	// Write new null terminator
	*(end+1) = 0;

	return str;
}

ssize_t ft_iphashmap_load(struct ft_iphashmap * this, const char * filename)
{
	assert(this != NULL);

	char line[256];

	FILE * f = fopen(filename, "r");
	if (f == NULL)
	{
		FT_WARN_ERRNO(errno, "fopen(%s)", filename);
		return -1;
	}

	ssize_t count = 0;
	while (fgets(line, sizeof(line), f))
	{
		char * c = ft_iphashmap_trim_whitespace(line);
		if (strlen(c) == 0) continue;
		if (c[0] == '#') continue;
		if (c[0] == ';') continue;

		struct ft_iphashmap_entry * e = ft_iphashmap_add_p(this, AF_UNSPEC, c);
		if (e == NULL)
		{
			FT_WARN("Cannot parse '%s' from '%s' file", line, c);
		}
		else count += 1;
	}
	fclose(f);

	return count;
}
