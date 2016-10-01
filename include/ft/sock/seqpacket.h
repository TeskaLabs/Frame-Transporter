#ifndef FT_SOCK_SEQPACKET_H_
#define FT_SOCK_SEQPACKET_H_

struct ft_seqpacket;

struct ft_seqpacket_delegate
{
	struct ft_frame * (*get_read_frame)(struct ft_seqpacket *); // If NULL, then simple frame will be used
	bool (*read)(struct ft_seqpacket *, struct ft_frame * frame); // True as a return value means, that the frame has been handed over to upstream protocol

	void (*connected)(struct ft_seqpacket *); // Called when connect() is successfully finished; can be NULL

	void (*fini)(struct ft_seqpacket *);

	void (*error)(struct ft_seqpacket *); // Don't use this for close() or shutdown, it will be done automatically and it can lead to wierd results
};

struct ft_seqpacket
{
	// Common fields
	struct ft_seqpacket_delegate * delegate;
	struct ft_context * context;

	struct
	{
		bool connecting : 1;
		bool active : 1;  // Yes - we initiated connection using connect(), No - accept()

		bool read_partial : 1; // When yes, read() callback is triggered for any incoming data
		bool read_throttle : 1;
		bool read_shutdown : 1;  // Socket is read-wise connected

		bool write_shutdown : 1; // Socket is write-wise connected
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
	ev_tstamp read_shutdown_at;
	ev_tstamp write_shutdown_at;

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

bool ft_seqpacket_init(struct ft_seqpacket * this, struct ft_seqpacket_delegate * delegate, struct ft_context * context, int fd);
bool ft_seqpacket_accept(struct ft_seqpacket *, struct ft_seqpacket_delegate * delegate, struct ft_listener * listening_socket, int fd, const struct sockaddr * peer_addr, socklen_t peer_addr_len);
bool ft_seqpacket_connect(struct ft_seqpacket *, struct ft_seqpacket_delegate * delegate, struct ft_context * context, const struct addrinfo * addr);
void ft_seqpacket_fini(struct ft_seqpacket *);

bool ft_seqpacket_write(struct ft_seqpacket *, struct ft_frame * frame);

void ft_seqpacket_diagnose(struct ft_seqpacket *);

///

enum ft_seqpacket_cntl_codes
{
	FT_SEQPACKET_READ_START     = 0x0001,
	FT_SEQPACKET_READ_STOP      = 0x0002,
	FT_SEQPACKET_READ_PAUSE     = 0x0004,  // Start read throttling
	FT_SEQPACKET_READ_RESUME    = 0x0008,  // Stop read throttling

	FT_SEQPACKET_WRITE_START    = 0x0010,
	FT_SEQPACKET_WRITE_STOP     = 0x0020,
	FT_SEQPACKET_WRITE_SHUTDOWN = 0x0040,  // Submit write shutdown
};


static inline bool ft_seqpacket_cntl(struct ft_seqpacket * this, const int control_code)
{
	assert(this != NULL);

	bool _ft_seqpacket_cntl_read_start(struct ft_seqpacket *);
	bool _ft_seqpacket_cntl_read_stop(struct ft_seqpacket *);
	bool _ft_seqpacket_cntl_read_throttle(struct ft_seqpacket *, bool throttle);
	
	bool _ft_seqpacket_cntl_write_start(struct ft_seqpacket *);
	bool _ft_seqpacket_cntl_write_stop(struct ft_seqpacket *);
	bool _ft_seqpacket_cntl_write_shutdown(struct ft_seqpacket *);

	bool ok = true;
	if ((control_code & FT_SEQPACKET_READ_START) != 0) ok &= _ft_seqpacket_cntl_read_start(this);
	if ((control_code & FT_SEQPACKET_READ_STOP) != 0) ok &= _ft_seqpacket_cntl_read_stop(this);
	if ((control_code & FT_SEQPACKET_READ_PAUSE) != 0) ok &= _ft_seqpacket_cntl_read_throttle(this, true);
	if ((control_code & FT_SEQPACKET_READ_RESUME) != 0) ok &= _ft_seqpacket_cntl_read_throttle(this, false);
	if ((control_code & FT_SEQPACKET_WRITE_START) != 0) ok &= _ft_seqpacket_cntl_write_start(this);
	if ((control_code & FT_SEQPACKET_WRITE_STOP) != 0) ok &= _ft_seqpacket_cntl_write_stop(this);
	if ((control_code & FT_SEQPACKET_WRITE_SHUTDOWN) != 0) ok &= _ft_seqpacket_cntl_write_shutdown(this);
	return ok;
}

///

static inline bool ft_seqpacket_is_shutdown(struct ft_seqpacket * this)
{
	return ((this->flags.read_shutdown) && (this->flags.write_shutdown));
}

static inline void ft_seqpacket_set_partial(struct ft_seqpacket * this, bool read_partial)
{
	assert(this != NULL);
	this->flags.read_partial = read_partial;
}

#endif // FT_SOCK_SEQPACKET_H_
