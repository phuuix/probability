/*************************************************************************/
/* The Dooloo kernel                                                     */
/* Copyright (C) 2004-2006 Xiong Puhui (Bearix)                          */
/* All Rights Reserved.                                                  */
/*                                                                       */
/* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS  */
/* THE PROPERTY OF DOOLOO RTOS DEVELOPMENT TEAM                          */
/*                                                                       */
/*************************************************************************/

/*************************************************************************
 * 
 * FILE                                       VERSION
 *   sem.c                                     0.3.0
 *
 * COMPONENT
 *   Kernel
 * 
 * DESCRIPTION
 *   semaphore support
 *
 * CHANGELOG
 *   AUTHOR         DATE                    NOTES
 *   Bearix         2006-8-20               Version 0.3.0
 *************************************************************************/ 


#include <config.h>

#include <sys/types.h>
#include <ipc.h>
#include <task.h>
#include <list.h>
#include <assert.h>
#include <bsp.h>
#include <ctype.h>

#include "probability.h"

/*
 * initialize a semaphore
 *   sem: semaphore
 *   name: name of semaphore
 *   flags: flags of semaphore
 *   value: semaphore's init value
 *   return: ROK on success, RERROR on failed
 */
int sem_initialize(sem_t *sem, int value)
{
	if(sem)
	{
		uint32_t f;

		f = bsp_fsave();
		
		sem->value = value;
		blockq_init(&sem->taskq);
		sem->flag = IPC_FLAG_VALID;
		bsp_frestore(f);

		return ROK;
	}

	return RERROR;
}


/*
 * cleanup a semaphore
 *   sem: semaphore
 */
void sem_destroy(sem_t *sem)
{
	struct dtask *t;
	uint32_t f;

	if(sem && (sem->flag & IPC_FLAG_VALID))
	{
		f = bsp_fsave();
		sem->flag = 0;

		/* wake up all tasks in semaphore's block task queue */
		while((t = blockq_select(&sem->taskq)))
		{
			task_wakeup_noschedule(TASK_T(t));
		}
		task_schedule();
		bsp_frestore(f);
	}
}


/*
 * P operation
 *   s: semaphore
 *   timeout: p's max wait time
 *     <0 -- wait forever; 0 -- no wait; >0 -- wait for timeout ticks
 *   retrun: RERROR on failed, RTIMEOUT on timeout, RAGAIN on busy, RSIGNAL when wakeup by signal, ROK on success.
 */
int sem_pend(sem_t *s, int timeout)
{
	uint32_t f;
	int r = RERROR;

	if(s && (s->flag & IPC_FLAG_VALID))
	{
		f = bsp_fsave();
		s->value--;
		if(s->value < 0)
		{
			if(timeout == 0)
			{
				r = RAGAIN;
				s->value ++;
			}
			else
			{
				current->wakeup_cause = RERROR;
				/* current task will sleep on sem */
				if(timeout > 0)
				{
					assert(current->t_delay.param[0] == TASK_T(current));
					assert(!(current->flags & TASK_FLAGS_DELAYING));
					ptimer_start(&sysdelay_queue, &current->t_delay, timeout);
					current->flags |= TASK_FLAGS_DELAYING;
				}
				task_block(&(s->taskq));
				
				switch ((r = current->wakeup_cause))
				{
				case ROK:
					break;
					
				case RTIMEOUT:
				case RSIGNAL:
				default:
					/* up value if task is wakeup for timeout or signal or destroy */
					s->value++;
					break;
				}
			}
		}
		else
			r = ROK;
		bsp_frestore(f);
	}
	
	return r;
}

/*
 * V operation
 *   s: semahore
 *   return: ERROR on failed; ROK on success
 */
int sem_post(sem_t *s)
{
	uint32_t f;
	int r = RERROR;
	struct dtask *t;

	if(s && (s->flag & IPC_FLAG_VALID))
	{
		r = ROK;
		f = bsp_fsave();
		s->value++;
		if(s->value <= 0)
		{
			/* wakeup task */
			t = blockq_select(&(s->taskq));
			if(t)
			{
				t->wakeup_cause= ROK;
				task_wakeup(TASK_T(t));
			}
		}
		bsp_frestore(f);
	}
	return r;
}


