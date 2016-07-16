#ifndef __LIBSCCMN_SOCK_EST_H__
#define __LIBSCCMN_SOCK_EST_H__

struct established_socket;

struct ft_stream_delegate
{
	struct frame * (*get_read_frame)(struct established_socket *); // If NULL, then simple frame will be used
	bool (*read)(struct established_socket *, struct frame * frame); // True as a return value means, that the frame has been handed over to upstream protocol

	void (*connected)(struct established_socket *); // Called when connect() is successfully established; can be NULL

	void (*fini)(struct established_socket *);

	void (*error)(struct established_socket *);
};

struct established_socket
{
	// Common fields
	struct ft_stream_delegate * delegate;
	struct context * context;

	struct
	{
		unsigned int read_shutdown : 1;  // Socket is read-wise connected
		unsigned int write_shutdown : 1; // Socket is write-wise connected
		unsigned int write_open : 1;      // Write queue is open for adding new frames
		unsigned int write_ready : 1;     // We can write to the socket (no need to wait for EV_WRITE)
		unsigned int connecting : 1;
		unsigned int active : 1;  // Yes - we initiated connection using connect(), No - accept()
		unsigned int read_partial : 1; // When yes, read() callback is triggered for any incoming data
		unsigned int ssl_status : 2; // 0 - disconnected; 1 - in handshake; 2 - established
		unsigned int ssl_server : 1; // Yes - we are SSL server (SSL_accept will be used in accept), No - we are SSL client (SSL_connect will be used)

		unsigned int read_throttle : 1;
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
		unsigned long ssl_error; // ERR_peek_error()
	} error;

	// Input
	struct ev_io read_watcher;
	struct frame * read_frame;
	unsigned int read_events;

	// Output
	struct ev_io write_watcher;
	struct frame * write_frames; // Queue of write frames 
	struct frame ** write_frame_last;
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

	// Custom data field
	void * data;
};

bool established_socket_init_accept(struct established_socket *, struct ft_stream_delegate * delegate, struct listening_socket * listening_socket, int fd, const struct sockaddr * peer_addr, socklen_t peer_addr_len);
bool established_socket_init_connect(struct established_socket *, struct ft_stream_delegate * delegate, struct context * context, const struct addrinfo * addr);
void established_socket_fini(struct established_socket *);

void established_socket_read_start(struct established_socket *);
void established_socket_read_stop(struct established_socket *);
void established_socket_read_throttle(struct established_socket *, bool throttle);

void established_socket_write_start(struct established_socket *);
void established_socket_write_stop(struct established_socket *);

bool established_socket_write(struct established_socket *, struct frame * frame);
bool established_socket_write_shutdown(struct established_socket *);

static inline bool established_socket_is_shutdown(struct established_socket * this)
{
	return ((this->flags.read_shutdown) && (this->flags.write_shutdown));
}

static inline void established_socket_set_read_partial(struct established_socket * this, bool read_partial)
{
	assert(this != NULL);
	this->flags.read_partial = read_partial;
}

bool established_socket_ssl_enable(struct established_socket *, SSL_CTX *ctx);

void established_socket_diag(struct established_socket *);

///

static inline struct established_socket * established_socket_from_ssl(SSL * ssl)
{
    return SSL_get_ex_data(ssl, libsccmn_config.sock_est_ssl_ex_data_index);
}

// This function is to be used within SSL_CTX_set_verify() callback
static inline struct established_socket * established_socket_from_x509_store_ctx(X509_STORE_CTX * ctx)
{
    SSL * ssl = X509_STORE_CTX_get_ex_data(ctx, SSL_get_ex_data_X509_STORE_CTX_idx());
    assert(ssl != NULL);
    return established_socket_from_ssl(ssl);
}

#endif // __LIBSCCMN_SOCK_EST_H__
