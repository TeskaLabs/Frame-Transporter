#ifndef FT_EXIT_H_
#define FT_EXIT_H_

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

void ft_exit_invoke_all(struct ft_exit * list,  struct ft_context * context, enum ft_exit_phase phase);

#endif // FT_EXIT_H_
