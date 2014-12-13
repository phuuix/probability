/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
 *
 * Copyright (c) Puhui Xiong <phuuix@163.com>
 * @file
 *   list table APIs
 *
 * @History
 *   AUTHOR         DATE           NOTES
 */

#include <list.h>
#include <assert.h>

/*
 * init a double circle linked list
 */
void dllist_init(dllist_node_t *list)
{
	assert(list);
	
	list->prev = list;
	list->next = list;
}


/*
 * insert a node after list head
 */
void dllist_insert(dllist_node_t *list, dllist_node_t *node)
{
	assert(list);
	assert(node);
	
	node->prev = list;
	node->next = list->next;
	list->next->prev = node;
	list->next = node;
}

/*
 * remove a node from list
 */
void dllist_remove(dllist_node_t *list, dllist_node_t *node)
{
	assert(node);
	assert(node->prev);
	assert(node->next);
	
	node->prev->next = node->next;
	node->next->prev = node->prev;
}

/*
 * append a node to the end of list
 */
void dllist_append(dllist_node_t *list, dllist_node_t *node)
{
	assert(list);
	assert(node);
	
	node->prev = list->prev;
	node->next = list;
	list->prev->next = node;
	list->prev = node;
}

int dllist_isempty(dllist_node_t *list)
{
	assert(list);

	return (list->next == list);
}

