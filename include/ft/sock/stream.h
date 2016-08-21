#ifndef FT_SOCK_STREAM_H_
#define FT_SOCK_STREAM_H_

struct ft_stream;

struct ft_stream_delegate
{
	struct ft_frame * (*get_read_frame)(struct ft_stream *); // If NULL, then simple frame will be used
	bool (*read)(struct ft_stream *, struct ft_frame * frame); // True as a return value means, that the frame has been handed over to upstream protocol

	void (*connected)(struct ft_stream *); // Called when connect() is successfully finished; can be NULL

	void (*fini)(struct ft_stream *);

	void (*error)(struct ft_stream *); // Don't use this for close() or shutdown, it will be done automatically and it can lead to wierd results
};

struct ft_stream
{
	// Common fields
	struct ft_stream_delegate * delegate;
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

		bool ssl_server : 1; // Yes - we are SSL server (SSL_accept will be used in accept), No - we are SSL client (SSL_connect will be used)
		bool ssl_hsconf: 1; // Yes - handshake direction has been configured (accept/connect)
		unsigned int ssl_status : 2; // 0 - disconnected; 1 - in handshake; 2 - established
	} flags;

	int ai_family;
	int ai_socktype; // Always SOCK_STREAM
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
		unsigned long ssl_error; // ERR_peek_error()
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

	// SSL
	SSL *ssl;

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

bool ft_stream_accept(struct ft_stream *, struct ft_stream_delegate * delegate, struct ft_listener * listening_socket, int fd, const struct sockaddr * peer_addr, socklen_t peer_addr_len);
bool ft_stream_connect(struct ft_stream *, struct ft_stream_delegate * delegate, struct ft_context * context, const struct addrinfo * addr);
void ft_stream_fini(struct ft_stream *);

bool ft_stream_write(struct ft_stream *, struct ft_frame * frame);

bool ft_stream_enable_ssl(struct ft_stream *, SSL_CTX *ctx);

void ft_stream_diagnose(struct ft_stream *);

///

enum ft_stream_cntl_codes
{
	FT_STREAM_READ_START     = 0x0001,
	FT_STREAM_READ_STOP      = 0x0002,
	FT_STREAM_READ_PAUSE     = 0x0004,  // Start read throttling
	FT_STREAM_READ_RESUME    = 0x0008,  // Stop read throttling

	FT_STREAM_WRITE_START    = 0x0010,
	FT_STREAM_WRITE_STOP     = 0x0020,
	FT_STREAM_WRITE_SHUTDOWN = 0x0040,  // Submit write shutdown
};


static inline bool ft_stream_cntl(struct ft_stream * this, const int control_code)
{
	assert(this != NULL);

	bool _ft_stream_cntl_read_start(struct ft_stream *);
	bool _ft_stream_cntl_read_stop(struct ft_stream *);
	bool _ft_stream_cntl_read_throttle(struct ft_stream *, bool throttle);
	
	bool _ft_stream_cntl_write_start(struct ft_stream *);
	bool _ft_stream_cntl_write_stop(struct ft_stream *);
	bool _ft_stream_cntl_write_shutdown(struct ft_stream *);

	bool ok = true;
	if ((control_code & FT_STREAM_READ_START) != 0) ok &= _ft_stream_cntl_read_start(this);
	if ((control_code & FT_STREAM_READ_STOP) != 0) ok &= _ft_stream_cntl_read_stop(this);
	if ((control_code & FT_STREAM_READ_PAUSE) != 0) ok &= _ft_stream_cntl_read_throttle(this, true);
	if ((control_code & FT_STREAM_READ_RESUME) != 0) ok &= _ft_stream_cntl_read_throttle(this, false);
	if ((control_code & FT_STREAM_WRITE_START) != 0) ok &= _ft_stream_cntl_write_start(this);
	if ((control_code & FT_STREAM_WRITE_STOP) != 0) ok &= _ft_stream_cntl_write_stop(this);
	if ((control_code & FT_STREAM_WRITE_SHUTDOWN) != 0) ok &= _ft_stream_cntl_write_shutdown(this);
	return ok;
}

///

static inline bool ft_stream_is_shutdown(struct ft_stream * this)
{
	return ((this->flags.read_shutdown) && (this->flags.write_shutdown));
}

static inline void ft_stream_set_partial(struct ft_stream * this, bool read_partial)
{
	assert(this != NULL);
	this->flags.read_partial = read_partial;
}

///

static inline struct ft_stream * ft_stream_from_ssl(SSL * ssl)
{
    return SSL_get_ex_data(ssl, ft_config.stream_ssl_ex_data_index);
}

// This function is to be used within SSL_CTX_set_verify() callback
static inline struct ft_stream * ft_stream_from_x509_store_ctx(X509_STORE_CTX * ctx)
{
    SSL * ssl = X509_STORE_CTX_get_ex_data(ctx, SSL_get_ex_data_X509_STORE_CTX_idx());
    assert(ssl != NULL);
    return ft_stream_from_ssl(ssl);
}

#endif // FT_SOCK_STREAM_H_
