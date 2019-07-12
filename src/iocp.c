#include "_ft_internal.h"

bool ft_iocp_watcher_init(struct ft_iocp_watcher * this, struct ft_iocp_watcher_delegate * delegate, void * data) {
	assert(this != NULL);
	assert(delegate != NULL);

	this->overlapped.hEvent = NULL;
	this->delegate = delegate;
	this->data = data;

	return true;
}
