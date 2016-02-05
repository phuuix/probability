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
 *   task management related routines
 *
 * @History
 *   AUTHOR         DATE                 NOTES
 *   puhuix           2006-8-28             introduce bitmap O(1) scheduler, adapt for process
 */

#include <config.h>
#include <assert.h>
#include <task.h>
#include <list.h>
#include <ptimer.h>
#include <bsp.h>
#include <slab.h>
#include <string.h>
#include <stdlib.h>
#include <psignal.h>

#include "journal.h"
#include "probability.h"

struct dtask systask[MAX_TASK_NUMBER];	/* task structure */
struct dtask *current;				/* current task */
readyq_t sysready_queue;			/* ready queue */
static dllist_node_t sysdelay_slot[MAX_TASK_DELAY_NUMBER];
ptimer_table_t sysdelay_queue;		/* delay task queue */
uint32_t sysschedule_flags;

static uint32_t active_ints;
static void (*task_schedule_hook)(struct dtask *, struct dtask *);

extern char _irq_stack_start[];


void task_set_schedule_hook(void (*hook)(struct dtask *, struct dtask *))
{
	task_schedule_hook = hook;
}


/*
 * called when system interrupt incoming
 * this function isn't used if bsp_task_switch==bsp_task_switch_interrupt
 */
void sys_interrupt_enter(uint32_t irq)
{
	active_ints++;

#ifdef INCLUDE_PMCOUNTER
	PMC_PEG_COUNTER(PMC_sys32_counter[PMC_U32_nInterruptInCurrentTick], 1);
#endif
}

/*
 * called after isr function is called
 * this function isn't used if bsp_task_switch==bsp_task_switch_interrupt
 */
void sys_interrupt_exit(uint32_t irq)
{
	uint16_t stack_bottom = *(uint16_t *)_irq_stack_start;
	
	active_ints--;

	assert(stack_bottom == 8995);
	assert(active_ints == 0);
}

uint32_t sys_get_active_int()
{
    return active_ints;
}

/*
 * lock task schedule
 */
void task_lock()
{
	sysschedule_flags |= SCHEDULE_FLAGS_LOCK;
}

/*
 * unlock task schedule
 */
void task_unlock()
{
	sysschedule_flags &= ~SCHEDULE_FLAGS_LOCK;

	task_schedule();
}

/* this callback always runs in interrupt-prohibit mode */
static int32_t task_timeout(void *timer, uint32_t t, uint32_t param2)
{
	struct dtask *task = &systask[t];

	assert(task->flags & TASK_FLAGS_DELAYING);

	/* reset delaying flag */
	// task_undelay(t);
	task->flags &= ~TASK_FLAGS_DELAYING;
	
	/* add task into sysready_queue, but don't schedule right now */
	if(task->taskq)
		blockq_dequeue(task->taskq, task);
	readyq_enqueue(&sysready_queue, task);
	task->state = TASK_STATE_READY;
	task->wakeup_cause = RTIMEOUT;

	return 0;
}

/*
 * exit current task and switch to a new task. 
 * Warning: Don't exit the current task if you are in a ciritical region.
 */
void task_exit()
{
	uint32_t f;

	struct dtask *task;
	
	f = bsp_fsave();
	task = current;

	/* remove task from it's queue */
	if(task->state == TASK_STATE_READY)
		readyq_dequeue(&sysready_queue, task);
	else if(task->state == TASK_STATE_BLOCK)
		blockq_dequeue(task->taskq, task);

	/* remove task from delay queue */
	if(task->flags & TASK_FLAGS_DELAYING)
		task_undelay(TASK_T(task));

	task->state = TASK_STATE_DEAD;
	bsp_frestore(f);

#ifdef INCLUDE_JOURNAL
	journal_task_exit(task);
#endif
#ifdef INCLUDE_PMCOUNTER
	PMC_PEG_COUNTER(PMC_sys32_counter[PMC_U32_nTask],-1);
#endif

	/* free task's stack */
	if(!(task->flags & TASK_FLAGS_STATICSTACK))
		free((void *)task->stack_base);
	
	/* switch to new task */
	task_schedule();
}

/*-----------------------------------------------------------------------*/
/* create a task(CAN'T be called in interrupt)                           */
/*   name: task's name                                                   */
/*   task: task's main routine                                           */
/*   parm: main routine's parameters                                     */
/*   stack_base: stack base pointer                              */
/*   stack_size: task's stack size                                       */
/*   pri: priority                                                       */
/*   slice: task's all timeslice                                         */
/*   flags: control flags                                                */
/*   return: task id on success, -1 on failed                            */
/*-----------------------------------------------------------------------*/
task_t task_create(char *name, void (*task_func)(void *), void *parm, char *stack_base,
	size_t stack_size, uint8_t pri, uint32_t slice, uint32_t flags)
{
	uint32_t f;
	int i;
	struct dtask *task = NULL;

	if(pri >= MAX_PRIORITY_LEVEL)
		pri = pri/(256/MAX_PRIORITY_LEVEL);

	if(name == NULL || task_func == NULL)
		return RERROR;

	if(stack_size <= 100)
		return RERROR;
	
	/* find an unused task control block */
	f = bsp_fsave();
	for(i=1; i<MAX_TASK_NUMBER; i++)
	{
		if(systask[i].state == TASK_STATE_DEAD)
		{
			task = &systask[i];
			memset(task, 0, sizeof(struct dtask));
			task->state = TASK_STATE_SUSPEND;
			break;
		}
	}
	bsp_frestore(f);
	if(task == NULL)
		return RERROR;

	if(stack_base == NULL)
		stack_base = malloc(stack_size);
	else
		flags |= TASK_FLAGS_STATICSTACK;
	
	if(stack_base)
	{
		memset(stack_base, TASK_STACK_INIT_CHAR, stack_size);
		task->sp = bsp_task_init(i, task_func, parm, (char *)(stack_base+stack_size-4), task_exit);

		strncpy((char *)systask[i].name, name, TASK_NAME_MAX_LENGTH-1);
		task->flags = flags;
		task->priority = pri;
		task->pri_origin = pri;
		task->timeslice_all = slice;
		task->timeslice_left = slice;
		task->stack_base = stack_base;
		task->stack_size = stack_size;
		task->taskq = NULL;
		task->errcode = 0;
		task->t_delay.onexpired_func = task_timeout;
		task->t_delay.param[0] = i;

#ifdef INCLUDE_JOURNAL
		journal_task_create(task);
#endif
#ifdef INCLUDE_PMCOUNTER
		PMC_PEG_COUNTER(PMC_sys32_counter[PMC_U32_nTask],1);
#endif

		return i;
	}
	else
	{
		f = bsp_fsave();
		task->state = TASK_STATE_DEAD;
		bsp_frestore(f);
		return RERROR;
	}
}

/*
 * destroy certain task and switch to a new task. 
 *   t: task
 *   return: 0 -- OK
 */
int task_destroy(task_t t)
{
	uint32_t f;
	struct dtask *task;

	if(t >= MAX_TASK_NUMBER)
		return -1;

	task = &systask[t];
	
	f = bsp_fsave();
	if(task->critical_regions > 0)
	{
		bsp_frestore(f);
		return -1;
	}
	
	if(task == current)
	{
		task_exit();
		bsp_frestore(f);
		return 0;
	}
	
	switch(task->state)
	{
		case TASK_STATE_DEAD:
			bsp_frestore(f);
			return -1;
		case TASK_STATE_SUSPEND:
		case TASK_STATE_READY:
		case TASK_STATE_BLOCK:
			/* remove task from it's queue */
			if(task->state == TASK_STATE_READY)
				readyq_dequeue(&sysready_queue, task);
			else if(task->state == TASK_STATE_BLOCK)
				blockq_dequeue(task->taskq, task);

			/* remove task from delay queue */
			if(task->flags & TASK_FLAGS_DELAYING)
				task_undelay(t);

			/* set task's state to dead */
			task->state = TASK_STATE_DEAD;
			break;
	}
	bsp_frestore(f);

	/* free task's stack */
	if(!(task->flags | TASK_FLAGS_STATICSTACK))
		free((void *)task->stack_base);

	return 0;
}

/*-----------------------------------------------------------------------*/
/* resume a suspened task, also a created task                           */
/*   t: task id                                                          */
/*-----------------------------------------------------------------------*/
void task_resume(task_t t)
{
	if(t >= MAX_TASK_NUMBER)
		return;
	
	task_resume_noschedule(t);
	task_schedule();
}

/* 
 * start a task: insert task into sysready_queue, but do not schedule
 *   t: task
 */
void task_resume_noschedule(task_t t)
{
	uint32_t f;

	assert(t<MAX_TASK_NUMBER);

	f = bsp_fsave();
	if(systask[t].state == TASK_STATE_SUSPEND)
	{
		assert(systask[t].taskq == NULL);
		readyq_enqueue(&sysready_queue, &systask[t]);
	    systask[t].state = TASK_STATE_READY;
	}
	bsp_frestore(f);
}

/*-----------------------------------------------------------------------*/
/* suspend a task                                                        */
/*   t: task id                                                          */
/*	return: 0 -- OK                                                        */
/*-----------------------------------------------------------------------*/
int task_suspend(task_t t)
{
	uint32_t f;

	struct dtask *task;

	if(t >= MAX_TASK_NUMBER)
		return -1;

	task = &systask[t];

	f = bsp_fsave();
	/* can't suspend the current task */
	if(task == current)
	{
		bsp_frestore(f);
		return -1;
	}

	/* can't suspend the task in critical region */
	if(task->critical_regions > 0)
	{
		bsp_frestore(f);
		return -1;
	}

	/* remove task from it's queue */
	if(task->state == TASK_STATE_READY)
		readyq_dequeue(&sysready_queue, task);
	else if(task->state == TASK_STATE_BLOCK)
		blockq_dequeue(task->taskq, task);

	task->state = TASK_STATE_SUSPEND;

	bsp_frestore(f);
	return ROK;
}

/*-----------------------------------------------------------------------*/
/* sleep current task to wait queue                                      */
/* (Task specialed routine)                                              */
/*   tqueue: the queue that task sleep to                                */
/*-----------------------------------------------------------------------*/
void task_block(blockq_t * tqueue)
{
	uint32_t f;
	
	f = bsp_fsave();
	task_block_noschedule(tqueue);
	task_schedule();
	bsp_frestore(f);
}

/* task_wakeup_noschedule() must be called in safe context (fsave) */
void task_block_noschedule(blockq_t * tqueue)
{
	struct dtask *task;

	task = current;
	assert(task->state == TASK_STATE_READY);

	task->state = TASK_STATE_BLOCK;
	readyq_dequeue(&sysready_queue, task);
	/* tqueue can be NULL */
	if(tqueue)
		blockq_enqueue(tqueue, task);
}

/*-----------------------------------------------------------------------*/
/* wake up a task                                                        */
/*   t: task id                                                          */
/*-----------------------------------------------------------------------*/
void task_wakeup(task_t t)
{
	uint32_t f;
	f = bsp_fsave();
	task_wakeup_noschedule(t);
	task_schedule();
	bsp_frestore(f);
}

/* task_wakeup_noschedule() must be called in safe context (fsave) */
void task_wakeup_noschedule(task_t t)
{
	struct dtask *task;

	assert(t<MAX_TASK_NUMBER);
	task = &systask[t];

	if(task->state == TASK_STATE_BLOCK)
	{		
		/* add task into sysready_queue */
		blockq_dequeue(task->taskq, task);
		readyq_enqueue(&sysready_queue, task);
		task->state = TASK_STATE_READY;

		/* if task is delaying, undelay it */
		if(task->flags & TASK_FLAGS_DELAYING)
			task_undelay(t);
	}
}

/*-----------------------------------------------------------------------*/
/* yield cpu to task that has the same priority at least                 */
/*-----------------------------------------------------------------------*/
void task_yield()
{
	uint32_t f;
	struct dtask *task;

	f = bsp_fsave();
	task = current;
	if(task->state == TASK_STATE_READY)
	{
		readyq_dequeue(&sysready_queue, task);
		readyq_enqueue(&sysready_queue, task);

		task_schedule();
	}
	bsp_frestore(f);
}

/*-----------------------------------------------------------------------*/
/* set task's option                                                     */
/*   t: task                                                     */
/*   opt: option                                                         */
/*   value: option's value                                               */
/*   size: value's size                                                  */
/*   return: 0 on success                                                 */
/*-----------------------------------------------------------------------*/
int task_setopt(task_t t, uint32_t opt, void* value, size_t size)
{
	uint32_t f;
	struct dtask *task;

	if(t<0 || t>=MAX_TASK_NUMBER)
		return -1;
	task = &systask[t];

	switch(opt)
	{
	case TASK_OPTION_PRIORITY:
		f = bsp_fsave();
		task->priority = *((uint8_t*)value);
		if(task->taskq)
		{
			if(task->state == TASK_STATE_READY){
				readyq_dequeue(&sysready_queue, task);
				readyq_enqueue(&sysready_queue, task);
			}
			else if(task->state == TASK_STATE_BLOCK){
				blockq_t *q = task->taskq;
				blockq_dequeue(q, task);
				blockq_enqueue(q, task);
			}
		}
		bsp_frestore(f);
		break;
		
	default:
		return -1;
	}
	return 0;
}

/*-----------------------------------------------------------------------*/
/* task schedule                                                         */
/*-----------------------------------------------------------------------*/
void task_schedule()
{
	uint32_t f;
	struct dtask *from, *to;
	
	f = bsp_fsave();

	/* when init current is NULL, this time we can't schedule */
	if(current == NULL)
		goto sche_end;

    /* SCHEDULE_FLAGS_LOCK or SCHEDULE_FLAGS_ONGOING is set */
    if(sysschedule_flags)
        goto sche_end;

	/* if current task is the task who owns the highest priority */
	to = readyq_select(&sysready_queue);
	assert(to);
	if(current == to)
		goto sche_end;

	from = current;

#ifdef INCLUDE_JOURNAL
	journal_task_switch(from, to);
#endif

#ifdef INCLUDE_PMCOUNTER
	PMC_PEG_COUNTER(PMC_sys32_counter[PMC_U32_nTaskSwitch],1);

	PMC_PEG_COUNTER(to->PMC_task32[PMC_U32_Task_nTaskSwitch], 1);

	bsp_gettime(&to->time_in_sec, &to->time_in_ns);
	from->time_accumulate_c_ns += to->time_in_ns - from->time_in_ns;
#endif

	/* call the task_schedule_hook() in from task's context */
	if(task_schedule_hook)
		(*task_schedule_hook)(from, to);
	
    /* In some implementation, the bsp_task_switch isn't a real task switch but a trigger of trap.
     * So we can't assign the current here, instead the current has to be assign inside the function
     * bsp_task_switch or the trap function
     */
	// current = to;
	
	if(active_ints)
		bsp_task_switch_interrupt((uint32_t)from, (uint32_t)to);
	else
		bsp_task_switch((uint32_t)from, (uint32_t)to);

	/* whenever task is wake up, check the signal */
#ifdef INCLUDE_SIGNAL
	{
		int signum;

		if((signum = sig_pending()))
		{
			struct dtask *task = current;
			sigset_t old_set;

			/* save mask */
			old_set = task->sigctrl.t_hold;

			/* add mask to t_hold */
			task->sigctrl.t_hold |= ((1L << (signum-1)) | task->sigctrl.t_action[signum-1].sa_mask);
			
			bsp_frestore(f);
			/* TODO: enable interrupt */
			
			/* call signal handler */
			if(task->sigctrl.t_action[signum-1].sa_handler)
				(*(task->sigctrl.t_action[signum-1].sa_handler))(signum);

			f = bsp_fsave();

			/* restore old mask */
			task->sigctrl.t_hold = old_set;
		}
	}
#endif /* INCLUDE_SIGNAL */

sche_end:
	bsp_frestore(f);
}

/*-----------------------------------------------------------------------*/
/* delay a task for a certain time                                       */
/*   tick: the ticks delayed                                             */
/*   return: 0 on success                                   */
/*-----------------------------------------------------------------------*/
int task_delay(uint32_t tick)
{
	int ret;
	uint32_t f;
	
	if(tick > 0){
		f = bsp_fsave();
		assert(current->t_delay.onexpired_func == task_timeout);
		assert(current->t_delay.param[0] == TASK_T(current));
		ret = ptimer_start(&sysdelay_queue, &current->t_delay, tick);
		if(ret == ROK){
			current->flags |= TASK_FLAGS_DELAYING;
			task_block(NULL);
		}
		bsp_frestore(f);
	}
	return ret;
}

/*
 * undelay a task, but no do scheduling
 *   t: task
 *   return: 0 on success
 */
int task_undelay(task_t t)
{
	struct dtask *task;

	assert(t<MAX_TASK_NUMBER);
	task = &systask[t];
	assert((task->flags & TASK_FLAGS_DELAYING));

	ptimer_cancel(&sysdelay_queue, &task->t_delay);
	task->flags &= ~TASK_FLAGS_DELAYING;

	return 0;
}

/*
 * init system task queue
 * this function must be called before any other task_xxx()
 */
void task_init()
{
	uint32_t f;

	f = bsp_fsave();
	
	/* init system ready task queue */
	readyq_init(&sysready_queue);

	/* init system delay task queue */
	ptimer_init_nomalloc(&sysdelay_queue, MAX_TASK_DELAY_NUMBER, sysdelay_slot);

	/* set current to NULL, then task_schedule() can be called,
	 * even system ready queue is empty */
	current = NULL;

	sysschedule_flags = 0;
	active_ints = 0;

	bsp_frestore(f);
}


