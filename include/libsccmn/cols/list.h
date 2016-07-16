#ifndef __FT_COLS_LIST_H__
#define __FT_COLS_LIST_H__

struct ft_list;

typedef void (* ft_list_on_remove_callback)(struct ft_list * list, struct ft_node * node);

struct ft_list
{
    size_t size;

    ft_list_on_remove_callback on_remove_callback;

    struct ft_node * head;
    struct ft_node * tail;
};

bool ft_list_init(struct ft_list *, ft_list_on_remove_callback on_remove_callback);
void ft_list_fini(struct ft_list *);

void ft_list_on_remove(struct ft_list * this, ft_list_on_remove_callback callback);

void ft_list_add(struct ft_list *, struct ft_node * node);

bool ft_list_remove(struct ft_list * , struct ft_node * node);
bool ft_list_remove_first(struct ft_list * );
bool ft_list_remove_last(struct ft_list * );
void ft_list_remove_all(struct ft_list *);

typedef void (* ft_list_each_callback)(struct ft_node * node, void * data);
static inline void ft_list_each(struct ft_list * this, ft_list_each_callback callback, void * data)
{
	for (struct ft_node * node = this->head; node != NULL; node = node->right)
	{
		callback(node, data);
	}
}

#endif // __FT_COLS_LIST_H__
