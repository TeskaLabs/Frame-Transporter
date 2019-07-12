#ifndef FT_IOCP_H_
#define FT_IOCP_H_

struct ft_iocp_watcher;

struct ft_iocp_watcher_delegate {
	void (*completed)(struct ft_context *, struct ft_iocp_watcher *, DWORD n_bytes, ULONG_PTR completion_key);
	void (*error)(struct ft_context *, struct ft_iocp_watcher *, DWORD error, DWORD n_bytes, ULONG_PTR completion_key);
};

struct ft_iocp_watcher {
	OVERLAPPED overlapped; // Must be first in the structure
	const struct ft_iocp_watcher_delegate * delegate;
	void * data;
};

bool ft_iocp_watcher_init(struct ft_iocp_watcher *, struct ft_iocp_watcher_delegate * delegate, void * data);


#endif // FT_IOCP_H_
