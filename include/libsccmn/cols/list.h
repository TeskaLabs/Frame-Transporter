#ifndef __FT_COLS_LIST_H__
#define __FT_COLS_LIST_H__

struct ft_list_node
{
	struct ft_list_node * next;
	struct ft_list_node * prev;

	char payload[];
};

typedef void (*ft_list_node_fini_cb)(struct ft_list_node * node);;

struct ft_list
{
    size_t size;

    struct ft_list_node * head;
    struct ft_list_node * tail;

    ft_list_node_fini_cb node_fini_cb;
};

bool ft_list_init(struct ft_list *, ft_list_node_fini_cb node_fini_cb);
void ft_list_fini(struct ft_list *);

void ft_list_add(struct ft_list *, struct ft_list_node * node);

bool ft_list_remove(struct ft_list * , struct ft_list_node * node);
bool ft_list_remove_first(struct ft_list * );
bool ft_list_remove_last(struct ft_list * );
void ft_list_remove_all(struct ft_list *);

typedef void (* ft_list_each_cb)(struct ft_list_node * node, void * data);
void ft_list_each(struct ft_list *, ft_list_each_cb callback, void * data);


static inline struct ft_list_node * ft_list_node_new(size_t payload_size)
{
	struct ft_list_node * this = malloc(sizeof(struct ft_list_node) + payload_size);
	if (this != NULL)
	{
		this->prev = NULL;
		this->next = NULL;
	}
	return this;
}

static inline void ft_list_node_del(struct ft_list_node * this, struct ft_list * list)
{
	list->node_fini_cb(this);
	free(this);
}

#endif // __FT_COLS_LIST_H__
