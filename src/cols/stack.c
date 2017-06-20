#include "../_ft_internal.h"

bool ft_stack_init(struct ft_stack * this, ft_stack_on_remove_callback on_remove_callback)
{
	assert(this != NULL);

	struct ft_stack_head head_init = {0, NULL};
    this->head = ATOMIC_VAR_INIT(head_init);
    this->size = ATOMIC_VAR_INIT(0);
    this->on_remove_callback = ATOMIC_VAR_INIT(on_remove_callback);

	return(true);
}

void ft_stack_fini(struct ft_stack * this)
{
	assert(this != NULL);
	ft_stack_clear(this);
}

size_t ft_stack_clear(struct ft_stack * this)
{
	assert(this != NULL);
	size_t cnt = 0;
	while (this->size > 0)
	{
		struct ft_stack_node * node = ft_stack_pop(this);
		if (node == NULL) continue;
		cnt += 1;
		if (this->on_remove_callback != NULL) this->on_remove_callback(this, node);
		ft_stack_node_del(node);
	}
	return cnt;
}


static void _ft_stack_push(_Atomic struct ft_stack_head * head, struct ft_stack_node *node)
{
	struct ft_stack_head next, orig = atomic_load(head);
	do {
		node->next = orig.node;
		next.aba = orig.aba + 1;
		next.node = node;
	} while (!atomic_compare_exchange_weak(head, &orig, next));
}

bool ft_stack_push(struct ft_stack * this, struct ft_stack_node * node)
{
	assert(this != NULL);
	_ft_stack_push(&this->head, node);
	atomic_fetch_add(&this->size, 1);
	return true;
}


static struct ft_stack_node * _ft_stack_pop(_Atomic struct ft_stack_head * head)
{
	struct ft_stack_head next, orig = atomic_load(head);
	do {
		if (orig.node == NULL)
			return NULL;
		next.aba = orig.aba + 1;
		next.node = orig.node->next;
	} while (!atomic_compare_exchange_weak(head, &orig, next));
	return orig.node;
}

struct ft_stack_node * ft_stack_pop(struct ft_stack * this)
{
	assert(this != NULL);

	struct ft_stack_node * node = _ft_stack_pop(&this->head);
    if (node == NULL) return NULL;

    atomic_fetch_sub(&this->size, 1);
    return node;
}
