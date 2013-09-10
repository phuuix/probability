/*************************************************************************/
/* The Dooloo kernel                                                     */
/* Copyright (C) 2004-2006 Xiong Puhui (Bearix)                          */
/* All Rights Reserved.                                                  */
/*                                                                       */
/* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS  */
/* THE PROPERTY OF DOOLOO RTOS DEVELOPMENT TEAM                          */
/*                                                                       */
/*************************************************************************/

/*************************************************************************/
/*                                                                       */
/* FILE                                       VERSION                    */
/*   buffer.c                                  0.3.0                     */
/*                                                                       */
/* COMPONENT                                                             */
/*   Kernel                                                              */
/*                                                                       */
/* DESCRIPTION                                                           */
/*                                                                       */
/*                                                                       */
/* CHANGELOG                                                             */
/*   AUTHOR         DATE                    NOTES                        */
/*   Bearix         2006-8-27               Version 0.3.0                */
/*************************************************************************/ 

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

