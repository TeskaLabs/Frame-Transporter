#ifndef FT_COLS_STACK_H_
#define FT_COLS_STACK_H_

// Lock-free (!) stack
// 
// A lock-free stack doesn’t require the use of mutex locks.
// More generally, it’s a data structure that can be accessed from multiple threads without blocking.
// Lock-free data structures will generally provide better throughput than mutex locks.
// And it’s usually safer, because there’s no risk of getting stuck on a lock that will never be freed, such as a deadlock situation.
// On the other hand there’s additional risk of starvation (livelock), where a thread is unable to make progress.
//
// Heavily inspired by http://nullprogram.com/blog/2014/09/02/ (Chris Wellons)

struct ft_stack;

struct ft_stack_node
{
	struct ft_stack_node * next;

	char data[];
};

static inline struct ft_stack_node * ft_stack_node_new(size_t payload_size)
{
	struct ft_stack_node * this = malloc(sizeof(struct ft_stack_node) + payload_size);
	if (this != NULL)
	{
		this->next = NULL;
	}
	return this;
}

static inline void ft_stack_node_del(struct ft_stack_node * this)
{
	free(this);
}


struct ft_stack_head
{
	uintptr_t aba;
	struct ft_stack_node * node;
};

typedef void (* ft_stack_on_remove_callback)(struct ft_stack * stack, struct ft_stack_node * node);

struct ft_stack
{
	struct ft_stack_node *node_buffer;
	_Atomic struct ft_stack_head head;
	_Atomic size_t size;
	ft_stack_on_remove_callback on_remove_callback;
};

static inline size_t ft_stack_size(struct ft_stack * this)
{
	assert(this != NULL);
	return atomic_load(&this->size);
}

bool ft_stack_init(struct ft_stack *, ft_stack_on_remove_callback on_remove_callback);
void ft_stack_fini(struct ft_stack *);

size_t ft_stack_clear(struct ft_stack *);

bool ft_stack_push(struct ft_stack *, struct ft_stack_node * node);
struct ft_stack_node * ft_stack_pop(struct ft_stack *);

#endif // FT_COLS_STACK_H_
