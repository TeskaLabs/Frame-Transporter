#include "../_ft_internal.h"

static void ft_wnp_dgram_complete_connected(struct ft_wnp_dgram * this);
extern struct ft_iocp_watcher_delegate ft_wnp_dgram_read_delegate;
extern struct ft_iocp_watcher_delegate ft_wnp_dgram_write_delegate;

///

bool ft_wnp_dgram_init(struct ft_wnp_dgram * this, struct ft_wnp_dgram_delegate * delegate, struct ft_context * context, const char * pipename)
{
	assert(this != NULL);
	assert(delegate != NULL);
	assert(delegate->read != NULL);

	this->delegate = delegate;
	this->context = context;

	this->pipe = CreateNamedPipe(
		pipename,                 // pipe name 
		PIPE_ACCESS_DUPLEX |      // read/write access 
		FILE_FLAG_OVERLAPPED,     // overlapped mode 
		PIPE_TYPE_MESSAGE |       // message-type pipe 
		PIPE_READMODE_MESSAGE |   // message-read mode 
		PIPE_WAIT |               // blocking mode 
		PIPE_REJECT_REMOTE_CLIENTS,
		PIPE_UNLIMITED_INSTANCES,
		65536,                    // output buffer size 
		65536,                    // input buffer size 
		NMPWAIT_USE_DEFAULT_WAIT, // client time-out 
		NULL                      // default security attributes 
	);
	if (this->pipe == INVALID_HANDLE_VALUE) 
	{
		FT_ERROR_WINERROR_P("CreateNamedPipe() failed");
		return false;
	}

	HANDLE iocp_handle = CreateIoCompletionPort(
		this->pipe,
		context->iocp_handle,
		(ULONG_PTR) this,
		0 // Ignored in this case
	);

	if (iocp_handle == NULL) {
		FT_ERROR_WINERROR_P("CreateIoCompletionPort() failed");
		CloseHandle(this->pipe);
		this->pipe = INVALID_HANDLE_VALUE;
		return false;
	}

	assert(iocp_handle == context->iocp_handle);

	// Read setup
	this->read_frame = NULL;
	this->flags.read_state = ft_wnp_state_INIT;
	ft_iocp_watcher_init(&this->read_watcher, &ft_wnp_dgram_read_delegate, this);

	// Write setup
	ft_iocp_watcher_init(&this->write_watcher, &ft_wnp_dgram_write_delegate, this);

	return true;
}


void ft_wnp_dgram_fini(struct ft_wnp_dgram * this)
{
	assert(this != NULL);

	if ((this->flags.read_state == ft_wnp_state_CONNECTING) || (this->flags.read_state == ft_wnp_state_STARTED)) {
		ft_wnp_dgram_stop(this);
	}

	if (this->delegate->fini != NULL) {
		this->delegate->fini(this);
	}
	
	if (this->pipe != INVALID_HANDLE_VALUE) {
		BOOL ok = CloseHandle(this->pipe);
		if (!ok) FT_WARN_WINERROR_P("CloseHandle(pipe) failed");
		this->pipe = INVALID_HANDLE_VALUE;
	}

	if (this->read_frame != NULL) {
		ft_frame_return(this->read_frame);
		this->read_frame = NULL;
	}
}


static bool ft_wnp_dgram_connect(struct ft_wnp_dgram * this) {
	assert(this != NULL);
	assert(this->flags.read_state == ft_wnp_state_INIT);

	BOOL ok = ConnectNamedPipe(this->pipe, &this->read_watcher.overlapped);
	if (!ok) {
		switch (GetLastError()) {
			// The overlapped connection in progress. 
			case ERROR_IO_PENDING: 
				break; 
 
			// Client is already connected, so signal an event. 
			case ERROR_PIPE_CONNECTED: 
				ft_wnp_dgram_complete_connected(this);
				break;

			default:
				FT_ERROR_WINERROR_P("ConnectNamedPipe() failed");
				return false;
		}
	} else {
		ft_wnp_dgram_complete_connected(this);
	}

	// Switching to connecting ...
	this->flags.read_state = ft_wnp_state_CONNECTING;
	return true;	
}


bool ft_wnp_dgram_start(struct ft_wnp_dgram * this){
	assert(this != NULL);


	switch (this->flags.read_state) {

		case ft_wnp_state_INIT:
			ft_wnp_dgram_connect(this);
			ft_ref(this->context);
			break;

		case ft_wnp_state_STOPPED:
			ft_wnp_dgram_connect(this);
			ft_ref(this->context);
			break;

		case ft_wnp_state_CONNECTING:
		case ft_wnp_state_STARTED:
			FT_WARN("Windows pipe is already started, cannot start");
			return false;

		case ft_wnp_state_CLOSED:
			FT_WARN("Windows pipe is already closed, cannot start");
			return false;
	}

	return true;
}


bool ft_wnp_dgram_stop(struct ft_wnp_dgram * this)
{
	assert(this != NULL);

	switch (this->flags.read_state) {

		case ft_wnp_state_INIT:
			FT_WARN("Windows pipe is only initialized, cannot stop");
			break;

		case ft_wnp_state_STOPPED:
			FT_WARN("Windows pipe is already stopped, cannot stop");
			break;

		case ft_wnp_state_CLOSED:
			FT_WARN("Windows pipe is already closed, cannot stop");
			return false;

		case ft_wnp_state_CONNECTING:
		case ft_wnp_state_STARTED:
			//TODO: This ...
			ft_unref(this->context);
			return false;
	}


	return true;
}


static void ft_wnp_dgram_complete_connected(struct ft_wnp_dgram * this) {
	assert((this->flags.read_state = ft_wnp_state_CONNECTING) || (this->flags.read_state = ft_wnp_state_INIT));

	if (this->delegate->connected != NULL) {
		this->delegate->connected(this);
	}

	this->flags.read_state = ft_wnp_state_STARTED;
}


static void ft_wnp_dgram_on_error(struct ft_context * context, struct ft_iocp_watcher * watcher, DWORD error, DWORD n_bytes, ULONG_PTR completion_key) {
	assert(watcher != NULL);
	struct ft_wnp_dgram * this = (struct ft_wnp_dgram *) watcher->data;
	assert(this != NULL);

	fprintf(stderr, "ft_wnp_dgram_on_error: %d\n", error);
}


static void ft_wnp_dgram_complete_read(struct ft_wnp_dgram * this, DWORD size) {
	assert(this->flags.read_state == ft_wnp_state_READY);

	struct ft_vec * vec = ft_frame_get_vec_at(this->read_frame, -1);
	assert(vec != NULL);
	assert(vec->position == 0);
	assert(size <= vec->capacity);

	ft_vec_set_position(vec, size);
	ft_vec_trim(vec);
	ft_vec_set_position(vec, 0);

	bool release = this->delegate->read(this, this->read_frame);
	if (release) {
		// The frame has been handled by an upstream protocol
		this->read_frame = NULL;
	}
}


static void ft_wnp_dgram_on_read(struct ft_context * context, struct ft_iocp_watcher * watcher, DWORD n_bytes, ULONG_PTR completion_key) {
	BOOL ok;

	assert(watcher != NULL);
	struct ft_wnp_dgram * this = (struct ft_wnp_dgram *) watcher->data;
	assert(this != NULL);

	switch (this->flags.read_state) {
		case ft_wnp_state_CONNECTING:
			ft_wnp_dgram_complete_connected(this);
			break;

		case ft_wnp_state_READY:
			ft_wnp_dgram_complete_read(this, n_bytes);
			break;

		default:
			FT_ERROR_P("Unknown/invalid state '%d'", this->flags.read_state);
	}

	while (this->flags.read_state == ft_wnp_state_READY) {
		// Setup a reading ...
		if (this->read_frame == NULL) {
			if (this->delegate->get_read_frame != NULL) {
				this->read_frame = this->delegate->get_read_frame(this);
			} else {
				this->read_frame = ft_pool_borrow(&context->frame_pool, FT_FRAME_TYPE_RAW_DATA);
			}
			if (this->read_frame == NULL) {
				FT_WARN("Cannot borrow a frame");
				return;
			}
			ft_frame_format_empty(this->read_frame);
		}

		assert (this->read_frame != NULL);

		// Add a new vector in the frame
		struct ft_vec * vec = ft_frame_append_max_vec(this->read_frame);
		if (vec == NULL) {
			FT_WARN("A frame is full, cannot add a new vector");
			return;
		}

		ok = ReadFile( 
			this->pipe,
			ft_vec_ptr(vec), ft_vec_remaining(vec), 
			&n_bytes, 
			&this->read_watcher.overlapped
		);

		if (ok == TRUE) {
			ft_wnp_dgram_complete_read(this, n_bytes);
			continue; // new round of the reading
		}
		
		switch (GetLastError()) {
			case ERROR_IO_PENDING: 
				// The overlapped reading in progress. 
				return; 
 
 			default:
				FT_ERROR_WINERROR_P("ReadFile() failed");
				return;
		}
	}
}

struct ft_iocp_watcher_delegate ft_wnp_dgram_read_delegate = {
	.completed = ft_wnp_dgram_on_read,
	.error = ft_wnp_dgram_on_error,
};


bool ft_wnp_dgram_write(struct ft_wnp_dgram * this, struct ft_frame * frame) {
	assert(this != NULL);
	assert(frame != NULL);

	BOOL ok;
	DWORD cbRet;

	struct ft_vec * vec = ft_frame_get_vec(frame);
	assert(vec != NULL);

	ok = WriteFile( 
		this->pipe,
		ft_vec_ptr(vec), ft_vec_remaining(vec), 
		&cbRet, 
		&this->write_watcher.overlapped
	);

	if (ok) {
		ft_frame_return(frame);
		return true;
	}

	fprintf(stderr, "WriteFile() -> %c - NOT IMPLEMENTED!!!\n", ok ? 'Y' : 'n');
	//TODO: This ... WriteFile is not completed, maybe b/c of error or due to an async operation

	return true;
}


static void ft_wnp_dgram_on_write(struct ft_context * context, struct ft_iocp_watcher * watcher, DWORD n_bytes, ULONG_PTR completion_key) {
	fprintf(stderr, "ft_wnp_dgram_on_write !!!\n");
}

struct ft_iocp_watcher_delegate ft_wnp_dgram_write_delegate = {
	.completed = ft_wnp_dgram_on_write,
	.error = ft_wnp_dgram_on_error,
};
