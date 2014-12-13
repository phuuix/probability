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
 *   mutex implementation
 *
 * @History
 *   AUTHOR         DATE                 NOTES
 *   puhuix           2006-8-31             modify API forms according to process adaption
 *                                                    use IPC api
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
 * initialize a mutex
 *   mtx: mutex
 *   return: ROK on success, RERROR on failed
 */
int mtx_initialize(mtx_t *mtx)
{
	if(mtx)
	{
		uint32_t f;

		f = bsp_fsave();
		
		mtx->value = 1;
		mtx->holdtimes = 0;
		mtx->owner = NULL;
		blockq_init(&mtx->taskq);
		mtx->flag = IPC_FLAG_VALID;

#ifdef INCLUDE_JOURNAL
		journal_ipc_init((ipc_t *)mtx);
#endif
		
#ifdef INCLUDE_PMCOUNTER
		PMC_PEG_COUNTER(PMC_sys32_counter[PMC_U32_nMutex], 1);
#endif

		bsp_frestore(f);
		
		return ROK;
	}

	return RERROR;
}


/*
 * cleanup a mutex
 *   mtx: mutex
 */
void mtx_destroy(mtx_t *mtx)
{
	struct dtask *t;
	uint32_t f;

	if(mtx && (mtx->flag & IPC_FLAG_VALID))
	{
		f = bsp_fsave();

#ifdef INCLUDE_JOURNAL
		journal_ipc_destroy((ipc_t *)mtx);
#endif
				
#ifdef INCLUDE_PMCOUNTER
		PMC_PEG_COUNTER(PMC_sys32_counter[PMC_U32_nMutex], -1);
#endif

		mtx->flag = 0;

		/* wake up all tasks in semaphore's block task queue */
		while((t = blockq_select(&mtx->taskq)))
		{
			task_wakeup_noschedule(TASK_T(t));
		}
		task_schedule();
		bsp_frestore(f);
	}
}


/*
 * mutex take
 *   m: mutex
 *   timeout: max wait time
 *     <0 -- wait forever; 0 -- no wait; >0 -- wait for timeout ticks
 *   retrun: RERROR on failed, RTIMEOUT on timeout, RAGAIN on busy, RSIGNAL when wakeup by signal, ROK on success.
 */
int mtx_pend(mtx_t *m, time_t timeout)
{
	uint32_t f;
	int r = RERROR;

	if(m && (m->flag & IPC_FLAG_VALID))
	{
		f = bsp_fsave();
		if(m->owner == current)	/* recursive holding */
		{
			m->holdtimes ++;
			r = ROK;
			bsp_frestore(f);
			return r;
		}

		m->value--;
		if(m->value < 0)
		{
			if(timeout == 0)
			{
				r = RAGAIN;
				m->value++;
				bsp_frestore(f);
			}
			else
			{
				/* To prevent priority inverse, we must rise mtx owner's priority */
				assert(m->owner);
				if(current->priority<m->owner->priority)
					task_setopt(TASK_T(m->owner), TASK_OPTION_PRIORITY, &(current->priority), sizeof(uint8_t));

				current->wakeup_cause = RERROR;
				if(timeout > 0){
					assert(current->t_delay.param[0] == TASK_T(current));
					assert(!(current->flags & TASK_FLAGS_DELAYING));
					ptimer_start(&sysdelay_queue, &current->t_delay, timeout);
					current->flags |= TASK_FLAGS_DELAYING;
				}
				task_block(&(m->taskq));
				bsp_frestore(f);

				r = RERROR;
				f = bsp_fsave();
				switch ((r = current->wakeup_cause))
				{
				case ROK:
					assert(m->holdtimes == 0);
					assert(m->owner == NULL);
					m->holdtimes ++;
					m->owner = current;
					break;
					
				case RTIMEOUT:
				case RSIGNAL:
				default:
					/* up value if task is wakeup for timeout or signal or destroy*/
					m->value++;

					/* FIX ME: Should we lower mtx owner's priority? */
					/* No need for it, this work is done in post operation */
					break;
				}
				bsp_frestore(f);
			}
		}
		else
		{
			assert(m->holdtimes == 0);
			assert(m->owner == NULL);
			m->holdtimes ++;
			m->owner = current;
			r = ROK;
			bsp_frestore(f);
		}
	}

	return r;
}

/*
 * V operation
 *   m: mutex
 *   return: 0 on failed; 1 on success
 */
int mtx_post(mtx_t *m)
{
	uint32_t f;
	int r = RERROR;
	struct dtask *t;
	
	if(m && (m->flag & IPC_FLAG_VALID))
	{
		r = ROK;
		f = bsp_fsave();
		
		/* only mutex's owner can post */
		if(m->owner != current)
		{
			bsp_frestore(f);
			return RERROR;
		}
		
		assert(m->holdtimes > 0);
		m->holdtimes --;
		if(m->holdtimes == 0)
		{
			m->value++;
			m->owner = NULL;

			/* restore task's priority */
			if(current->priority != current->pri_origin)
			{
				task_setopt(TASK_T(current), TASK_OPTION_PRIORITY, &(current->pri_origin), sizeof(uint8_t));
			}

			if(m->value <= 0)
			{
				/* wakeup task */
				t = blockq_select(&(m->taskq));
				if(t)
				{
					t->wakeup_cause = ROK;
					task_wakeup(TASK_T(t));
				}
			}
		}
		else
		{
			/* TODO: update current task's priority? */
			/* no, assume the hold time is relative short */
		}
		
		bsp_frestore(f);
	}
	
	return r;
}


