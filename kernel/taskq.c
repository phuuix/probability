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
 *   task queue implementation
 *
 * @History
 *   AUTHOR         DATE                 NOTES
 *   puhuix           
 */

#include <task.h>
#include <list.h>
#include <assert.h>
#include <string.h>

#include "journal.h"
#include "probability.h"

/* Bitmap related */

static const uint8_t task_map_table[8] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80};

static const uint8_t task_unmap_table[256] =
{
	0, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,		/* 0 */ 
	4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,		/* 1 */ 
	5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,		/* 2 */ 
	4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,		/* 3 */ 
	6, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,		/* 4 */ 
	4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,		/* 5 */
	5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,		/* 6 */ 
	4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,		/* 7 */ 
	7, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,		/* 8 */ 
	4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,		/* 9 */ 
	5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,		/* A */ 
	4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,		/* B */ 
	6, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,		/* C */ 
	4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,		/* D */ 
	5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,		/* E */ 
	4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0		/* F */ 
};


void bitmap_task_enter_ready(task_bitmap_t *bitmap, uint8_t prio)
{
	int groupidx = prio >> CONSTANT_SHIFT;
	int grouppri = prio & CONSTANT_MASK;
	
	bitmap->task_ready_priority_group[groupidx] |= task_map_table[grouppri >> 3];
	bitmap->task_ready_priority_table[prio >> 3] |= task_map_table[grouppri & 0x07];
}

void bitmap_task_leave_ready(task_bitmap_t *bitmap, uint8_t prio)
{
	int groupidx = prio >> CONSTANT_SHIFT;
	int grouppri = prio & CONSTANT_MASK;
	
	if((bitmap->task_ready_priority_table[prio >> 3] &= ~task_map_table[prio & 0x07]) == 0)
		bitmap->task_ready_priority_group[groupidx] &= ~task_map_table[grouppri >> 3];
}

uint8_t bitmap_task_highest_priority(task_bitmap_t *bitmap)
{
	int groupidx;
	int sub_prio_grp, sub_prio_offset;
	
	for(groupidx=0; groupidx < (MAX_PRIORITY_LEVEL>>CONSTANT_SHIFT); groupidx++)
	{
		if(bitmap->task_ready_priority_group[groupidx]) break;
	}
	
	sub_prio_grp = task_unmap_table[bitmap->task_ready_priority_group[groupidx]];
	sub_prio_offset = task_unmap_table[bitmap->task_ready_priority_table[(groupidx<<3) + sub_prio_grp]];
	return ((groupidx << CONSTANT_SHIFT) + (sub_prio_grp << 3) + sub_prio_offset);
}

void readyq_init(readyq_t *readyq)
{
	int i;
	
	assert(readyq);

	memset(readyq, 0, sizeof(readyq_t));
	
	for(i=0; i<MAX_PRIORITY_LEVEL; i++)
		dllist_init(&readyq->taskq_head[i]);
}

void readyq_enqueue(readyq_t *readyq, struct dtask *task)
{
	int isempty;

	assert(readyq);
	assert(task);
	
	isempty = dllist_isempty(&(readyq->taskq_head[task->priority]));
	if(isempty)
		bitmap_task_enter_ready(&readyq->bitmap, task->priority);

	dllist_append(&(readyq->taskq_head[task->priority]), &task->qnode);
	task->taskq = readyq;

#ifdef INCLUDE_PMCOUNTER
	PMC_PEG_COUNTER(PMC_sys32_counter[PMC_U32_nTaskReady],1);
#endif
}

void readyq_dequeue(readyq_t *readyq, struct dtask *task)
{
	int isempty;

	assert(readyq);
	assert(task);
	assert(task->taskq == readyq);

	dllist_remove(&(readyq->taskq_head[task->priority]), &task->qnode);
	task->taskq = NULL;

	isempty = dllist_isempty(&(readyq->taskq_head[task->priority]));
	if(isempty)
		bitmap_task_leave_ready(&readyq->bitmap, task->priority);

#ifdef INCLUDE_PMCOUNTER
	PMC_PEG_COUNTER(PMC_sys32_counter[PMC_U32_nTaskReady],-1);
#endif
}

struct dtask *readyq_select(readyq_t *readyq)
{
	int priority;
	struct dtask *task = NULL;
	int isempty;
	dllist_node_t *node;
	
	priority = bitmap_task_highest_priority(&readyq->bitmap);

	isempty = dllist_isempty(&(readyq->taskq_head[priority]));
	if(!isempty)
	{
		node = readyq->taskq_head[priority].next;
		task = container_of(node, dtask_t, qnode);
	}

	return task;
}

#if BLOCKQ_IS_SIZE_SENSTIVE

void blockq_init(blockq_t *blockq)
{
	assert(blockq);

	dllist_init(blockq);
}

void blockq_enqueue(blockq_t *blockq, struct dtask *task)
{
	dllist_node_t *ntmp, *node;
	struct dtask *ttmp;
	
	assert(blockq);
	assert(task);

	ntmp = blockq->next;
	while(ntmp != blockq)
	{
		
		ttmp = container_of(ntmp, dtask_t, qnode);

		if(ttmp->priority > task->priority)
			break;

		ntmp = ntmp->next;
	}

	node = &task->qnode;
	task->taskq = blockq;

	/* insert before ntmp */
	node->prev = ntmp->prev;
	node->next = ntmp;
	ntmp->prev->next = node;
	ntmp->prev = node;
}

void blockq_dequeue(blockq_t *blockq, struct dtask *task)
{
	dllist_node_t *node;
	
	assert(task);
	assert(task->taskq == blockq);

	if(blockq)
	{
		/* remove from queue */
		node = &task->qnode;
		dllist_remove(blockq, node);
		task->taskq = NULL;
		node->prev = NULL;
		node->next = NULL;
	}
}

struct dtask *blockq_select(blockq_t *blockq)
{
	struct dtask *task;
	int isempty;
	dllist_node_t *node;

	assert(blockq);

	isempty = dllist_isempty(blockq);
	if(!isempty)
	{
		node = blockq->next;
		task = container_of(node, dtask_t, qnode);
	}

	return isempty?NULL:task;
}

#endif /* BLOCKQ_IS_SIZE_SENSTIVE */

