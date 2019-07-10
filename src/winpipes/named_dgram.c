#include "../_ft_internal.h"

static void ft_wnp_dgram_read_watcher_cb(struct ft_context * context, struct ft_watcher * watcher);
static void ft_wnp_dgram_write_watcher_cb(struct ft_context * context, struct ft_watcher * watcher);

static void ft_wnp_dgram_complete_connected(struct ft_wnp_dgram * this);
static void ft_wnp_dgram_complete_read(struct ft_wnp_dgram * this, DWORD size);

bool ft_wnp_dgram_init(struct ft_wnp_dgram * this, struct ft_wnp_dgram_delegate * delegate, struct ft_context * context, const char * pipename)
{
	assert(this != NULL);
	assert(delegate != NULL);
	assert(delegate->read != NULL);

	this->delegate = delegate;

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


	// Read setup

	this->read_frame = NULL;
	this->flags.read_state = ft_wnp_state_INIT;
	this->read_watcher.next = NULL;
	this->read_watcher.data = this;
	this->read_watcher.callback = ft_wnp_dgram_read_watcher_cb;
	this->read_watcher.context = context;
	this->read_watcher.handle = CreateEvent( 
		NULL,    // default security attribute 
		TRUE,    // manual-reset event 
		TRUE,    // initial state = signaled 
		NULL     // unnamed event object 
	);
	if (this->read_watcher.handle == NULL)
	{
		FT_ERROR_WINERROR_P("CreateEvent()/read_watcher failed");
		CloseHandle(this->pipe);
		this->pipe = INVALID_HANDLE_VALUE;
		return false;
	}
	this->read_overlap.hEvent = this->read_watcher.handle;


	// Write setup
	this->write_watcher.next = NULL;
	this->write_watcher.data = this;
	this->write_watcher.callback = ft_wnp_dgram_write_watcher_cb;
	this->write_watcher.context = context;
	this->write_watcher.handle = CreateEvent( 
		NULL,    // default security attribute 
		TRUE,    // manual-reset event 
		TRUE,    // initial state = signaled 
		NULL     // unnamed event object 
	);
	if (this->write_watcher.handle == NULL)
	{
		FT_ERROR_WINERROR_P("CreateEvent()/write_watcher failed");
		CloseHandle(this->pipe);
		this->pipe = INVALID_HANDLE_VALUE;
		CloseHandle(this->read_watcher.handle);
		this->read_watcher.handle = NULL;
		return false;
	}
	this->write_overlap.hEvent = this->write_watcher.handle;

	return true;
}


void ft_wnp_dgram_fini(struct ft_wnp_dgram * this)
{
	assert(this != NULL);

	if (ft_wnp_dgram_is_started(this)) {
		ft_wnp_dgram_stop(this);
	}

	if (this->delegate->fini != NULL) {
		this->delegate->fini(this);
	}

	if (this->write_watcher.handle != NULL) {
		BOOL ok = CloseHandle(this->write_watcher.handle);
		if (!ok) FT_WARN_WINERROR_P("CloseHandle(write_watcher) failed");
		this->write_watcher.handle = NULL;
		this->write_overlap.hEvent = NULL;
	}

	if (this->read_watcher.handle != NULL) {
		BOOL ok = CloseHandle(this->read_watcher.handle);
		if (!ok) FT_WARN_WINERROR_P("CloseHandle(read_watcher) failed");
		this->read_watcher.handle = NULL;
		this->read_overlap.hEvent = NULL;
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


bool ft_wnp_dgram_start(struct ft_wnp_dgram * this)
{
	assert(this != NULL);

	switch (this->flags.read_state) {
		case ft_wnp_state_INIT: {
			BOOL ok = ConnectNamedPipe(this->pipe, &this->read_overlap);
			if (!ok) {
				switch (GetLastError()) {
					// The overlapped connection in progress. 
					case ERROR_IO_PENDING: 
						break; 
		 
					// Client is already connected, so signal an event. 
					case ERROR_PIPE_CONNECTED: 
						ok = SetEvent(this->read_watcher.handle);
						if (!ok) {
							FT_ERROR_WINERROR_P("SetEvent() failed");
							return false;
						}
						ft_wnp_dgram_complete_connected(this);
						break;

					default:
						FT_ERROR_WINERROR_P("ConnectNamedPipe() failed");
						return false;
				}
			}

			// Switching to connecting ...
			this->flags.read_state = ft_wnp_state_CONNECTING;
		}

		case ft_wnp_state_CONNECTING:
		case ft_wnp_state_READY:
			ft_context_watcher_add(this->read_watcher.context, &this->read_watcher);
			return true;

		case ft_wnp_state_CLOSED:
			return false;

	}
}


bool ft_wnp_dgram_stop(struct ft_wnp_dgram * this)
{
	assert(this != NULL);

	ft_context_watcher_remove(this->read_watcher.context, &this->read_watcher);

	return true;
}


bool ft_wnp_dgram_is_started(struct ft_wnp_dgram * this) {
	assert(this != NULL);
	return ft_context_watcher_is_added(this->read_watcher.context, &this->read_watcher);
}


static void ft_wnp_dgram_complete_connected(struct ft_wnp_dgram * this) {
	assert((this->flags.read_state = ft_wnp_state_CONNECTING) || (this->flags.read_state = ft_wnp_state_INIT));

	if (this->delegate->connected != NULL) {
		this->delegate->connected(this);
	}

	this->flags.read_state = ft_wnp_state_READY;
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


void ft_wnp_dgram_read_watcher_cb(struct ft_context * context, struct ft_watcher * watcher)
{
	BOOL ok;
	DWORD cbRet;

	struct ft_wnp_dgram * this = (struct ft_wnp_dgram *) watcher->data;
	
	ok = GetOverlappedResult( 
		this->pipe,          // handle to pipe 
		&this->read_overlap, // OVERLAPPED structure 
		&cbRet,              // bytes transferred 
		FALSE                // do not wait 
 	);
	if (!ok) switch (GetLastError()) {
		case ERROR_BROKEN_PIPE: 
			// Peer closed a connection, normal situation
			ft_context_watcher_remove(this->read_watcher.context, &this->read_watcher);
			this->flags.read_state = ft_wnp_state_CLOSED;
			return; 

		default:
			FT_ERROR_WINERROR_P("ConnectNamedPipe() failed");
			return;
	}

	switch (this->flags.read_state) {
		case ft_wnp_state_CONNECTING:
			ft_wnp_dgram_complete_connected(this);
			break;

		case ft_wnp_state_READY:
			ft_wnp_dgram_complete_read(this, cbRet);
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
				this->read_frame = ft_pool_borrow(&this->read_watcher.context->frame_pool, FT_FRAME_TYPE_RAW_DATA);
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
			&cbRet, 
			&this->read_overlap
		);

		if (ok == TRUE) {
			ft_wnp_dgram_complete_read(this, cbRet);
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
		&this->write_overlap
	);

	if (ok) {
		ft_frame_return(frame);
		return true;
	}

	fprintf(stderr, "WriteFile() -> %c - NOT IMPLEMENTED!!!\n", ok ? 'Y' : 'n');
	//TODO: This ... WriteFile is not completed, maybe b/c of error or due to an async operation

	return true;
}

static void ft_wnp_dgram_write_watcher_cb(struct ft_context * context, struct ft_watcher * watcher)
{

}


