#include "../all.h"


bool ft_list_init(struct ft_list * this, ft_list_node_fini_cb node_fini_cb)
{
	this->size = 0;
	this->node_fini_cb = node_fini_cb;

	this->head = NULL;
	this->tail = NULL;

	return true;
}


void ft_list_fini(struct ft_list * this)
{
	ft_list_remove_all(this);
}


void ft_list_add(struct ft_list * this, struct ft_list_node * node)
{
	if (this->size == 0)
	{
		this->head = node;
		this->tail = node;
	}

	else
	{
		node->prev = this->tail;
		this->tail->next = node;
		this->tail = node;
	}

	this->size += 1;
}


void ft_list_each(struct ft_list * this, ft_list_each_cb callback, void * data)
{
	for (struct ft_list_node * node = this->head; node != NULL; node = node->next)
	{
		callback(node, data);
	}
}

//

static struct ft_list_node * ft_list_unlink(struct ft_list * this, struct ft_list_node * node)
{
	if (node->prev != NULL)
		node->prev->next = node->next;

	if (node->prev == NULL)
		this->head = node->next;

	if (node->next == NULL)
		this->tail = node->prev;

	if (node->next != NULL)
		node->next->prev = node->prev;

	this->size -= 1;
	return node;
}

bool ft_list_remove(struct ft_list * this, struct ft_list_node * node)
{
	//TODO: First seek for node to confirm that it is a part of the list
	ft_list_unlink(this, node);
	ft_list_node_del(node, this);
	return true;
}

bool ft_list_remove_first(struct ft_list * this)
{
	if (this->size == 0) return false;
	struct ft_list_node * node = ft_list_unlink(this, this->head);
	ft_list_node_del(node, this);
	return true;
}

bool ft_list_remove_last(struct ft_list * this)
{
	if (this->size == 0) return false;
	struct ft_list_node * node = ft_list_unlink(this, this->tail);
	ft_list_node_del(node, this);
	return true;
}

void ft_list_remove_all(struct ft_list * this)
{
	while (this->head != NULL)
	{
		ft_list_remove_first(this);
	}
}
