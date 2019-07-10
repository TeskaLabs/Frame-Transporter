#ifndef FT_NAMED_DGRAM_H_
#define FT_NAMED_DGRAM_H_

// Windows Named Pipes
// datagram / message variant

struct ft_wnp_dgram;

struct ft_wnp_dgram_delegate
{
	struct ft_frame * (*get_read_frame)(struct ft_wnp_dgram *); // If NULL, then simple frame will be used

	void (*connected)(struct ft_wnp_dgram *);
	bool (*read)(struct ft_wnp_dgram *, struct ft_frame * frame); // True as a return value means, that the frame has been handed over to upstream protocol

	void (*fini)(struct ft_wnp_dgram *);

	void (*error)(struct ft_wnp_dgram *); // Don't use this for close() or shutdown, it will be done automatically and it can lead to wierd results
};

enum ft_wnp_state {
	ft_wnp_state_INIT = 0,
	ft_wnp_state_CONNECTING = 1,
	ft_wnp_state_READY = 2,
	ft_wnp_state_CLOSED = 3,
}; 

struct ft_wnp_dgram {
	const struct ft_wnp_dgram_delegate * delegate;

	HANDLE pipe;

	// Read part
	struct ft_watcher read_watcher;
	OVERLAPPED read_overlap;

	// Write part
	struct ft_watcher write_watcher;
	OVERLAPPED write_overlap;

	struct
	{
		unsigned int read_state : 2; // one of `enum ft_wnp_state`		
	} flags;

	struct ft_frame * read_frame;

	// User data
	void * data;
};

bool ft_wnp_dgram_init(struct ft_wnp_dgram *, struct ft_wnp_dgram_delegate * delegate, struct ft_context * context, const char * pipename);
void ft_wnp_dgram_fini(struct ft_wnp_dgram *);

bool ft_wnp_dgram_start(struct ft_wnp_dgram *);
bool ft_wnp_dgram_stop(struct ft_wnp_dgram *);
bool ft_wnp_dgram_is_started(struct ft_wnp_dgram *);

bool ft_wnp_dgram_write(struct ft_wnp_dgram *, struct ft_frame * frame);

#endif //FT_NAMED_DGRAM_H_

