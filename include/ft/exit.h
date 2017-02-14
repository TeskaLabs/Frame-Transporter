#ifndef FT_EXIT_H_
#define FT_EXIT_H_

// Implementation of termination / exit watcher
// Triggered when ft_context is about to exit event loop
// Application should remove all active watchers to ensure smooth event loop termination

enum ft_exit_phase
{
	FT_EXIT_PHASE_POLITE
	//TODO: More phases
};

struct ft_exit;
typedef void (*ft_exit_cb)(struct ft_exit *, struct ft_context * context, enum ft_exit_phase phase);

struct ft_exit
{
	struct ft_context * context;
	struct ft_exit * next;

	ft_exit_cb callback;
	void * data;
};

bool ft_exit_init(struct ft_exit * , struct ft_context * context, ft_exit_cb callback);
void ft_exit_fini(struct ft_exit * );

#endif // FT_EXIT_H_
