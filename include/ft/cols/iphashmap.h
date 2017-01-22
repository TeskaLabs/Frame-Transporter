#ifndef FT_COLS_IPHASHMAP_H_
#define FT_COLS_IPHASHMAP_H_

/*
The dictionary of IP addresses (IPv4 and IPv6)
*/

union ft_iphashmap_addr
{
	struct in_addr ip4;
	struct in6_addr ip6;
};

struct ft_iphashmap_entry
{
	struct ft_iphashmap_entry * next;

	short int family;
	union ft_iphashmap_addr addr;

	void * data;
};

struct ft_iphashmap
{
	int bucket_size;
	int count;
	struct ft_iphashmap_entry ** buckets;
};


bool ft_iphashmap_init(struct ft_iphashmap * , int initial_bucket_size);
void ft_iphashmap_fini(struct ft_iphashmap *);
//TODO: resize function

void ft_iphashmap_clear(struct ft_iphashmap *);

struct ft_iphashmap_entry * ft_iphashmap_add_ip4(struct ft_iphashmap * this, struct in_addr addr);
struct ft_iphashmap_entry * ft_iphashmap_get_ip4(struct ft_iphashmap * this, struct in_addr addr);
bool ft_iphashmap_pop_ip4(struct ft_iphashmap * this, struct in_addr addr, void ** data);

struct ft_iphashmap_entry * ft_iphashmap_add_ip6(struct ft_iphashmap * this, struct in6_addr addr);
struct ft_iphashmap_entry * ft_iphashmap_get_ip6(struct ft_iphashmap * this, struct in6_addr addr);
bool ft_iphashmap_pop_ip6(struct ft_iphashmap * this, struct in6_addr addr, void ** data);

// family can be one of AF_INET, AF_INET6 and AF_UNSPEC (autodetect)
struct ft_iphashmap_entry * ft_iphashmap_add_p(struct ft_iphashmap * this, int family, const void * src);
bool ft_iphashmap_pop_p(struct ft_iphashmap * this, int family, const void * src, void ** data);

struct ft_iphashmap_entry * ft_iphashmap_get_sa(struct ft_iphashmap * this, const struct sockaddr * addr, socklen_t addrlen);

ssize_t ft_iphashmap_load(struct ft_iphashmap *, const char * filename);

#endif //FT_COLS_IPHASHMAP_H_
