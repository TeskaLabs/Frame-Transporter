#ifndef FT_SOCK_DGRAM_H_
#define FT_SOCK_DGRAM_H_

struct ft_dgram;

struct ft_dgram_delegate
{
	struct ft_frame * (*get_read_frame)(struct ft_dgram *); // If NULL, then simple frame will be used
	bool (*read)(struct ft_dgram *, struct ft_frame * frame); // True as a return value means, that the frame has been handed over to upstream protocol

	void (*fini)(struct ft_dgram *);

	void (*error)(struct ft_dgram *); // Don't use this for close() or shutdown, it will be done automatically and it can lead to wierd results
};

struct ft_dgram
{
	// Common fields
	struct ft_dgram_delegate * delegate;
	struct ft_context * context;

	struct
	{
		bool active : 1;  // Yes - we initiated connection using connect(), No - accept()
		bool shutdown : 1;  // Socket is read-wise connected

		bool read_throttle : 1;

		bool write_open : 1;      // Write queue is open for adding new frames
		bool write_ready : 1;     // We can write to the socket (no need to wait for EV_WRITE)
	} flags;

	int ai_family;
	int ai_socktype;
	int ai_protocol;

	struct sockaddr_storage ai_addr;
	socklen_t ai_addrlen;

	ev_tstamp created_at;
	ev_tstamp connected_at;
	ev_tstamp shutdown_at;

	struct 
	{
		int sys_errno;
	} error;

	// Input
	struct ev_io read_watcher;
	struct ft_frame * read_frame;
	unsigned int read_events;

	// Output
	struct ev_io write_watcher;
	struct ft_frame * write_frames; // Queue of write frames 
	struct ft_frame ** write_frame_last;
	unsigned int write_events;

	// Statistics
	struct
	{
		unsigned int read_events;
		unsigned int write_events;
		unsigned int write_direct; //Writes without need of wait for EV_WRITE
		unsigned long read_bytes;
		unsigned long write_bytes;
	} stats;

	// Custom data fields
	void * protocol;
	void * data;
};

//bool ft_dgram_init(struct ft_dgram * this, struct ft_dgram_delegate * delegate, struct ft_context * context, int fd);
bool ft_dgram_bind(struct ft_dgram *, struct ft_dgram_delegate * delegate, struct ft_context * context, const struct addrinfo * addr);
bool ft_dgram_connect(struct ft_dgram *, struct ft_dgram_delegate * delegate, struct ft_context * context, const struct addrinfo * addr);
void ft_dgram_fini(struct ft_dgram *);

bool ft_dgram_write(struct ft_dgram *, struct ft_frame * frame);

void ft_dgram_diagnose(struct ft_dgram *);

///

enum ft_dgram_cntl_codes
{
	FT_DGRAM_READ_START     = 0x0001,
	FT_DGRAM_READ_STOP      = 0x0002,
	FT_DGRAM_READ_PAUSE     = 0x0004,  // Start read throttling
	FT_DGRAM_READ_RESUME    = 0x0008,  // Stop read throttling

	FT_DGRAM_WRITE_START    = 0x0010,
	FT_DGRAM_WRITE_STOP     = 0x0020,

	FT_DGRAM_SHUTDOWN = 0x0040,
};


static inline bool ft_dgram_cntl(struct ft_dgram * this, const int control_code)
{
	assert(this != NULL);

	bool _ft_dgram_cntl_read_start(struct ft_dgram *);
	bool _ft_dgram_cntl_read_stop(struct ft_dgram *);
	bool _ft_dgram_cntl_read_throttle(struct ft_dgram *, bool throttle);
	
	bool _ft_dgram_cntl_write_start(struct ft_dgram *);
	bool _ft_dgram_cntl_write_stop(struct ft_dgram *);
	bool _ft_dgram_cntl_shutdown(struct ft_dgram *);

	bool ok = true;
	if ((control_code & FT_DGRAM_READ_START) != 0) ok &= _ft_dgram_cntl_read_start(this);
	if ((control_code & FT_DGRAM_READ_STOP) != 0) ok &= _ft_dgram_cntl_read_stop(this);
	if ((control_code & FT_DGRAM_READ_PAUSE) != 0) ok &= _ft_dgram_cntl_read_throttle(this, true);
	if ((control_code & FT_DGRAM_READ_RESUME) != 0) ok &= _ft_dgram_cntl_read_throttle(this, false);
	if ((control_code & FT_DGRAM_WRITE_START) != 0) ok &= _ft_dgram_cntl_write_start(this);
	if ((control_code & FT_DGRAM_WRITE_STOP) != 0) ok &= _ft_dgram_cntl_write_stop(this);
	if ((control_code & FT_DGRAM_SHUTDOWN) != 0) ok &= _ft_dgram_cntl_shutdown(this);
	return ok;
}

#endif // FT_SOCK_DGRAM_H_
