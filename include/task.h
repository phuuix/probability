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
/*   task.h                                    0.3.0                     */
/*                                                                       */
/* COMPONENT                                                             */
/*   Kernel                                                              */
/*                                                                       */
/* DESCRIPTION                                                           */
/*   task header file                                                    */
/*                                                                       */
/* CHANGELOG                                                             */
/*   AUTHOR         DATE                    NOTES                        */
/*   Bearix         2006-8-20               Version 0.3.0                */
/*************************************************************************/ 


#ifndef __D_TASK_H__
#define __D_TASK_H__

#include <config.h>
#include <inttypes.h>
#include <sys/types.h>
#include <psignal.h>
#include <list.h>
#include <ptimer.h>

/* task's name length */
#define TASK_NAME_MAX_LENGTH	8

/* task's stack init value */
#define TASK_STACK_INIT_CHAR '#'

/* task's states */
#define TASK_STATE_DEAD		0
#define TASK_STATE_SUSPEND	1
#define TASK_STATE_READY	2
#define TASK_STATE_BLOCK	3

/* task's flags */
#define TASK_FLAGS_DONT_SLICE		0x01	/* can slice */
#define TASK_FLAGS_DELAYING			0x02	/* is delaying */
#define TASK_FLAGS_STATICSTACK		0x04	/* use static stack */

/* task's option */
#define TASK_OPTION_PRIORITY		0x01	/* priority */

/* schedule flags */
#define SCHEDULE_FLAGS_LOCK		0x01		/* lock schedule */
#define SCHEDULE_FLAGS_DOING	0x02		/* schedule is doing */

/* get a task pointer from a delay pointer */
#define DELAY_TO_TASK(d) \
	((struct dtask *)((unsigned char *)d-sizeof(struct dtask)+sizeof(struct dtask_delay)))

/* get task_t from a task pointer */
#define TASK_T(t) t-systask

typedef int task_t;

#define CONSTANT_SHIFT 6
#define CONSTANT_MASK 0x3F

typedef struct task_bitmap
{
	uint8_t task_ready_priority_table[MAX_PRIORITY_LEVEL/8];
	uint8_t task_ready_priority_group[MAX_PRIORITY_LEVEL>>CONSTANT_SHIFT];
}task_bitmap_t;

typedef struct bitmapq
{
	task_bitmap_t bitmap;
	dllist_node_t taskq_head[MAX_PRIORITY_LEVEL];
}bitmapq_t;

typedef bitmapq_t readyq_t;

/*
 * task struct
 * position of the first 3 fields (sp, prev, next) must not be modified
 * the last field must be delay
 */
struct dtask
{
	void *sp;							/* task's current stack pointer, used by BSP */

	dllist_node_t qnode;				/* list node to queue */
	void *taskq;						/* task's current ready or block or delay queue */

	uint8_t name[TASK_NAME_MAX_LENGTH];	/* task's name */
	uint8_t state;						/* task's state */
	uint8_t priority;					/* task's current priority */
	uint8_t pri_origin;					/* task's static priority */
	uint16_t flags;						/* task's flags */
	uint16_t wakeup_cause;				/* cause of wakeup */
	uint16_t critical_regions;			/* number of critical regions */

	void *stack_base;					/* task's base stack address */
	uint32_t stack_size;                /* task's stack size */

	uint32_t timeslice_all;				/* task's all timeslice */
	uint32_t timeslice_left;			/* the current ticks task has */

	uint32_t errcode;					/* error code for task */

#ifdef INCLUDE_SIGNAL
	struct signal_ctrl sigctrl;			/* signal control struct */
#endif /* INCLUDE_SIGNAL */

	void *context;						/* task's process context, used as extension for later*/

	ptimer_t t_delay;
};
typedef struct dtask dtask_t;

extern struct dtask *current;		/* systerm's current task */
extern struct dtask systask[];		/* systerm's all task */

extern readyq_t sysready_queue;	/* system's ready task queue */
extern ptimer_table_t sysdelay_queue;	/* system's delay task queue */

void readyq_init(readyq_t *readyq);
void readyq_enqueue(readyq_t *readyq, struct dtask *task);
void readyq_dequeue(readyq_t *readyq, struct dtask *task);
struct dtask *readyq_select(readyq_t *readyq);

#if BLOCKQ_IS_SIZE_SENSTIVE
typedef dllist_node_t blockq_t;
void blockq_init(blockq_t *blockq);
void blockq_enqueue(blockq_t *blockq, struct dtask *task);
void blockq_dequeue(blockq_t *blockq, struct dtask *task);
struct dtask *blockq_select(blockq_t *blockq);
#else
typedef bitmapq_t blockq_t;
#define blockq_init readyq_init;
#define blockq_enqueue readyq_enqueue;
#define blockq_dequeue readyq_dequeue;
#define blockq_select readyq_select;
#endif

task_t task_create(char *name, void (*task_func)(void *), void *parm, char *stack_base,
	size_t stack_size, uint8_t pri, uint32_t slice, uint32_t flags);
int task_destroy(task_t t);
void task_resume_noschedule(task_t t);
void task_exit();
int task_suspend(task_t t);
void task_resume(task_t t);
void task_resume_noschedule(task_t t);
void task_block(blockq_t * tqueue);
void task_block_noschedule(blockq_t * tqueue);
void task_wakeup(task_t t);
void task_wakeup_noschedule(task_t t);
void task_yield();
void task_schedule();
void task_init();
int task_delay(uint32_t tick);
int task_undelay(task_t t);

int task_getopt();
int task_setopt(task_t t, uint32_t opt, void* value, size_t size);

void task_lock();
void task_unlock();

void task_set_schedule_hook(void (*hook)(struct dtask *, struct dtask *));

void sys_interrupt_enter();
void sys_interrupt_exit();
#endif /* __D_TASK_H__ */

