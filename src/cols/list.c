#include "../_ft_internal.h"


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
	ft_list_clear(this);
}


void ft_list_on_remove(struct ft_list * this, ft_list_on_remove_callback callback)
{
	assert(this != NULL);
	this->on_remove_callback = callback;
}


void ft_list_append(struct ft_list * this, struct ft_list_node * node)
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


//

static void ft_list_unlink(struct ft_list * this, struct ft_list_node * node)
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

	node->prev = NULL;
	node->next = NULL;

	if (this->on_remove_callback != NULL) this->on_remove_callback(this, node);
	ft_list_node_del(node);
}

bool ft_list_remove(struct ft_list * this, struct ft_list_node * node)
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

void ft_list_clear(struct ft_list * this)
{
	while (this->head != NULL)
	{
		ft_list_remove_first(this);
	}
}
