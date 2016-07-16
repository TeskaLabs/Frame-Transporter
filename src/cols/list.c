#include "../all.h"


bool ft_list_init(struct ft_list * this, ft_list_on_remove_callback on_remove_callback)
{
	this->size = 0;

	this->head = NULL;
	this->tail = NULL;

	this->on_remove_callback = on_remove_callback;

	return true;
}


void ft_list_fini(struct ft_list * this)
{
	ft_list_remove_all(this);
}


void ft_list_on_remove(struct ft_list * this, ft_list_on_remove_callback callback)
{
	assert(this != NULL);
	this->on_remove_callback = callback;
}


void ft_list_add(struct ft_list * this, struct ft_node * node)
{
	if (this->size == 0)
	{
		this->head = node;
		this->tail = node;
	}

	else
	{
		node->left = this->tail;
		this->tail->right = node;
		this->tail = node;
	}

	this->size += 1;
}


//

static void ft_list_unlink(struct ft_list * this, struct ft_node * node)
{
	if (node->left != NULL)
		node->left->right = node->right;

	if (node->left == NULL)
		this->head = node->right;

	if (node->right == NULL)
		this->tail = node->left;

	if (node->right != NULL)
		node->right->left = node->left;

	this->size -= 1;

	node->left = NULL;
	node->right = NULL;

	if (this->on_remove_callback != NULL) this->on_remove_callback(this, node);
	ft_node_del(node);
}

bool ft_list_remove(struct ft_list * this, struct ft_node * node)
{
	//TODO: First seek for node to confirm that it is a part of the list
	ft_list_unlink(this, node);
	return true;
}

bool ft_list_remove_first(struct ft_list * this)
{
	if (this->size == 0) return false;
	ft_list_unlink(this, this->head);
	return true;
}

bool ft_list_remove_last(struct ft_list * this)
{
	if (this->size == 0) return false;
	ft_list_unlink(this, this->tail);
	return true;
}

void ft_list_remove_all(struct ft_list * this)
{
	while (this->head != NULL)
	{
		ft_list_remove_first(this);
	}
}
