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
 *   semaphore support
 *
 * @History
 *   AUTHOR         DATE                 NOTES
 *   
 */

#include <config.h>

#include <sys/types.h>
#include <ipc.h>
#include <task.h>
#include <list.h>
#include <assert.h>
#include <bsp.h>
#include <ctype.h>

#include "journal.h"
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

		SYS_FSAVE(f);

		sem->t_parent = TASK_T(current);
		sem->type = IPC_TYPE_SEMAPHORE;
		sem->value = value;
		blockq_init(&sem->taskq);
		sem->flag = IPC_FLAG_VALID;

#ifdef INCLUDE_JOURNAL
		journal_ipc_init((ipc_t *)sem);
#endif
					
#ifdef INCLUDE_PMCOUNTER
		PMC_PEG_COUNTER(PMC_sys32_counter[PMC_U32_nSemaphore], 1);
#endif

		SYS_FRESTORE(f);

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
		SYS_FSAVE(f);

#ifdef INCLUDE_JOURNAL
		journal_ipc_destroy((ipc_t *)sem);
#endif
						
#ifdef INCLUDE_PMCOUNTER
		PMC_PEG_COUNTER(PMC_sys32_counter[PMC_U32_nSemaphore], -1);
#endif

		sem->flag = 0;

		/* wake up all tasks in semaphore's block task queue */
		while((t = blockq_select(&sem->taskq)))
		{
			task_wakeup_noschedule(TASK_T(t));
		}
		task_schedule();
		SYS_FRESTORE(f);
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
		SYS_FSAVE(f);
#ifdef INCLUDE_JOURNAL
		journal_ipc_pend((ipc_t *)s, timeout);
#endif
		s->value--;
		if(s->value < 0)
		{
			if(timeout == 0)
			{
				r = RAGAIN;
				s->value ++;
				SYS_FRESTORE(f);
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
				SYS_FRESTORE(f);  // must restore flag here to let task switch occure

				SYS_FSAVE(f);
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
				SYS_FRESTORE(f);
			}
		}
		else
		{
			r = ROK;
			SYS_FRESTORE(f);
		}
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
	struct dtask *t = NULL;

	if(s && (s->flag & IPC_FLAG_VALID))
	{
		r = ROK;
		SYS_FSAVE(f);
		s->value++;
		if(s->value <= 0)
		{
			/* wakeup task */
			t = blockq_select(&(s->taskq));
		}

#ifdef INCLUDE_JOURNAL
		journal_ipc_post((ipc_t *)s, t);
#endif // INCLUDE_JOURNAL

		if(t)
		{
			t->wakeup_cause= ROK;
			task_wakeup(TASK_T(t));
		}
		
		SYS_FRESTORE(f);
	}
	return r;
}


