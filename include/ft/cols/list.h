#ifndef FT_COLS_LIST_H_
#define FT_COLS_LIST_H_

struct ft_list_node
{
	struct ft_list_node * next;
	struct ft_list_node * prev;

	char data[];
};

static inline struct ft_list_node * ft_list_node_new(size_t payload_size)
{
	struct ft_list_node * this = malloc(sizeof(struct ft_list_node) + payload_size);
	if (this != NULL)
	{
		this->next = NULL;
		this->prev = NULL;
	}
	return this;
}

static inline void ft_list_node_del(struct ft_list_node * this)
{
	free(this);
}

///

struct ft_list;

typedef void (* ft_list_on_remove_callback)(struct ft_list * list, struct ft_list_node * node);

struct ft_list
{
    size_t size;

    ft_list_on_remove_callback on_remove_callback;

    struct ft_list_node * head;
    struct ft_list_node * tail;

    void * data; // User field
};

bool ft_list_init(struct ft_list *, ft_list_on_remove_callback on_remove_callback);
void ft_list_fini(struct ft_list *);

void ft_list_on_remove(struct ft_list * this, ft_list_on_remove_callback callback);

void ft_list_add(struct ft_list *, struct ft_list_node * node);

bool ft_list_remove(struct ft_list * , struct ft_list_node * node);
bool ft_list_remove_first(struct ft_list * );
bool ft_list_remove_last(struct ft_list * );
void ft_list_remove_all(struct ft_list *);

#define FT_LIST_FOR(LIST, NODE) for (struct ft_list_node * NODE = (LIST)->head; NODE != NULL; NODE = NODE->next)

#endif // FT_COLS_LIST_H_
