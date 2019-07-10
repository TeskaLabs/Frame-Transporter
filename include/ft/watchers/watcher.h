#ifndef FT_WATCHER_WATCHER_H_
#define FT_WATCHER_WATCHER_H_

// Watcher is Abstract Base Class (ABC) for specific watchers implementations

struct ft_watcher;

typedef void (* ft_watcher_cb)(struct ft_context *, struct ft_watcher *);

struct ft_watcher {
	struct ft_watcher * next;
	struct ft_context * context;
	ft_watcher_cb callback;

	// This is Windows specific 
	HANDLE handle;

	void * data;
};

#endif // FT_WATCHER_WATCHER_H_
