#ifndef __FT_COLS_NODE_H__
#define __FT_COLS_NODE_H__

struct ft_node
{
	struct ft_node * left;
	struct ft_node * right;

	char data[];
};

static inline struct ft_node * ft_node_new(size_t payload_size)
{
	struct ft_node * this = malloc(sizeof(struct ft_node) + payload_size);
	if (this != NULL)
	{
		this->left = NULL;
		this->right = NULL;
	}
	return this;
}

static inline void ft_node_del(struct ft_node * this)
{
	free(this);
}

#endif // __FT_COLS_NODE_H__
