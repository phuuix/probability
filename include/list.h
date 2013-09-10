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
/*   list.h                                    0.3.0                     */
/*                                                                       */
/* COMPONENT                                                             */
/*   Kernel                                                              */
/*                                                                       */
/* DESCRIPTION                                                           */
/*   contain some list operator macro                                    */
/*                                                                       */
/* CHANGELOG                                                             */
/*   AUTHOR         DATE                    NOTES                        */
/*   Bearix         2006-8-20               Version 0.3.0                */
/*************************************************************************/ 

#ifndef __D_LIST_H__
#define __D_LIST_H__

/********************************************************************************/
/*                                  single list                                 */
/* a single list must have follow entry:                                        */
/*   next                                                                       */
/* example:                                                                     */
/* struct demo_list{                                                            */
/*     struct demo_list *next;                                                  */
/*     void *data;                                                              */
/* };                                                                           */
/********************************************************************************/
/* insert an element to list header */
#define SINGLE_LIST_INSERT(l, e) \
	(e)->next = *(l); \
	*(l) = e;

/* remove an element from list header */
#define SINGLE_LIST_REMOVE(l, e) \
	*(e) = *(l); \
	if(*(l) != NULL) \
		*(l) = (*(l))->next;

/********************************************************************************/
/*                                  queue                                       */
/* a queue struct must have follow entry:                                       */
/*    head, tail, length                                                        */
/* example:                                                                     */
/* struct demo_queue{                                                           */
/*     int head;                                                                */
/*     int tail;                                                                */
/*     int length;                                                              */
/*     void *data;                                                              */
/* };                                                                           */
/********************************************************************************/
/* init an queue */
#define QUEUE_INIT(q, l) \
	(q)->head = (q)->tail = 0; \
	(q)->length = l;

/* next queue element */
#define QUEUE_NEXT(q, i) 	(((i) + 1) % (q)->length)

/* queue is full */
#define QUEUE_FULL(q) 	(QUEUE_NEXT(q, (q)->head) == q->tail)

/* queue is empty */
#define QUEUE_EMPTY(q)	((q)->head == (q)->tail)

/* insert an element to queue's header */
#define QUEUE_INC(q) \
	if(!QUEUE_FULL(q)) {\
		(q)->head = QUEUE_NEXT(q, (q)->head); \
	}

/* remove an element from queue's tail */
#define QUEUE_DEC(q) \
	if(!QUEUE_EMPTY(q)) { \
		(q)->tail = QUEUE_NEXT(q, (q)->tail); \
	}

/********************************************************************************/
/*                   single linked list which has a head                        */
/********************************************************************************/
/* init single linked list */
#define SLINKLIST_INIT(LIST, NEXT) \
{ \
	(LIST)->NEXT = NULL; \
}

/* insert a node to list header */
#define SLINKLIST_INSERT(LIST, NEXT, NODE) \
{ \
	(NODE)->NEXT = (LIST)->NEXT; \
	(LIST)->NEXT = NODE; \
}

/* remove an element from list header */
#define SLINKLIST_REMOVE(LIST, NEXT, NODE) \
{ \
	*(NODE) = (LIST)->NEXT; \
	if(*(NODE)) \
	{ \
		(LIST)->NEXT = (*(NODE))->NEXT; \
		(*(NODE))->NEXT = NULL; \
	} \
}


/********************************************************************************/
/*                   double linked list which has a head                        */
/********************************************************************************/
/* init double linked list */
#define DC_LINKLIST_INIT(LIST, PREV, NEXT) \
{ \
	(LIST)->PREV = (LIST); \
	(LIST)->NEXT = (LIST); \
}

/* insert a node to list header */
#define DC_LINKLIST_INSERT(LIST, PREV, NEXT, NODE) \
{ \
	(NODE)->PREV = (LIST); \
	(NODE)->NEXT = (LIST)->NEXT; \
	(LIST)->NEXT->PREV = (NODE); \
	(LIST)->NEXT = (NODE); \
}

/* remove an element from list */
#define DC_LINKLIST_REMOVE(LIST, PREV, NEXT, NODE) \
{ \
	(NODE)->PREV->NEXT = (NODE)->NEXT; \
	(NODE)->NEXT->PREV = (NODE)->PREV; \
}

/* append an element to list */
#define DC_LINKLIST_APPEND(LIST, PREV, NEXT, NODE) \
{ \
	(NODE)->PREV = (LIST)->PREV; \
	(NODE)->NEXT = (LIST); \
	(LIST)->PREV->NEXT = (NODE); \
	(LIST)->PREV = (NODE); \
}


/*********************************************************************************
 * data and structure for function version double linked list
 *********************************************************************************/

typedef struct dllist_node
{
	struct dllist_node *prev;
	struct dllist_node *next;
}dllist_node_t;

#define DLLIST_EMPTY(list) ((list)->next == (list))

#define DLLIST_TAIL(list) ((list)->prev)
	
#define DLLIST_HEAD(list) ((list)->next)
	
#define DLLIST_IS_HEAD(list, node) \
	(((dllist_node_t *)(node) == (list)) ? 1 : 0)

void dllist_init(dllist_node_t *list);
void dllist_insert(dllist_node_t *list, dllist_node_t *node);
void dllist_remove(dllist_node_t *list, dllist_node_t *node);
void dllist_append(dllist_node_t *list, dllist_node_t *node);
int dllist_isempty(dllist_node_t *list);


#endif /* __D_LIST_H__ */
