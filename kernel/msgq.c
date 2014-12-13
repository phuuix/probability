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
 *   message queue implement
 *
 * @History
 *   AUTHOR         DATE                 NOTES
 *   
 */

#include <config.h>

#include <task.h>
#include <ipc.h>
#include <assert.h>
#include <memory.h>
#include <bsp.h>
#include <ctype.h>
#include <stdlib.h>

#include "journal.h"
#include "probability.h"


/*
 * initialize a message queue
 *   q: message queue
 *   bufsize: size of mem_buf
 *   buf: size of buf >= bufsize
 *   return: ROK on success, RERROR on failed
 */
int msgq_initialize(msgq_t *q, uint16_t bufsize, void *buf)
{
	uint32_t f;	

	if(q == NULL)
		return RERROR;

	if(bufsize <= 0)
		return RERROR;

	if(buf == NULL)
		buf = malloc(bufsize);
	
	if(buf == NULL)
		return RERROR;

	f = bsp_fsave();
	q->type = IPC_TYPE_MSGQ;
	q->flag = IPC_FLAG_VALID;
	blockq_init(&q->taskq);
	mem_buffer_init(&q->buf, buf, bufsize);

#ifdef INCLUDE_JOURNAL
	journal_ipc_init((ipc_t *)q);
#endif
			
#ifdef INCLUDE_PMCOUNTER
	PMC_PEG_COUNTER(PMC_sys32_counter[PMC_U32_nMessageQ], 1);
#endif

	bsp_frestore(f);
	
	return ROK;
}


/*
 * cleanup a message queue
 *   q: message queue
 */
void msgq_destroy(msgq_t *q)
{
	struct dtask *t;
	uint32_t f;

	if(q && (q->flag & IPC_FLAG_VALID))
	{
		f = bsp_fsave();

#ifdef INCLUDE_JOURNAL
		journal_ipc_destroy((ipc_t *)q);
#endif
				
#ifdef INCLUDE_PMCOUNTER
		PMC_PEG_COUNTER(PMC_sys32_counter[PMC_U32_nMessageQ], -1);
#endif

		q->flag = 0;

		/* wake up all tasks in msgq's block task queue */
		while((t = blockq_select(&q->taskq)))
		{
			task_wakeup_noschedule(TASK_T(t));
		}
		task_schedule();
		bsp_frestore(f);
	}
}


/*
 * send a message to messageq
 *   msgq: messageq
 *   msg: message
 *   msglen: length of message
 *   return: RERROR on failed; ROK on success
 */
int msgq_post(msgq_t *msgq, char *msg, size_t msglen)
{
	int ret = RERROR;
	uint32_t f;
	struct dtask *t;

	if(msgq && (msgq->flag & IPC_FLAG_VALID))
	{
		f = bsp_fsave();
		if(mem_buffer_put(&msgq->buf, msg, msglen))
		{
			ret = ROK;
			t = blockq_select(&(msgq->taskq));

			if(t)
			{
				t->wakeup_cause = ROK;
				task_wakeup(TASK_T(t));
			}
		}
		bsp_frestore(f);
	}

	return ret;
}


/*
 * receive an message from messageq
 *   msgq: messageq
 *   msg: message buffer
 *   msglen: message buffer length before receive, realy message length after receive
 *   timeout: receive's max wait time
 *     <0 -- wait forever; 0 -- no wait; >0 -- wait for timeout ticks
 *   retrun: RERROR on failed, RTIMEOUT on timeout, RAGAIN on busy, ROK on success.
 */
int msgq_pend(msgq_t *msgq, char *msg, size_t *msglen, int timeout)
{
	uint32_t f, len;
	int ret = RERROR;

	if(msgq && (msgq->flag & IPC_FLAG_VALID))
	{
		f = bsp_fsave();
		len = mem_buffer_get(&msgq->buf, msg, *msglen);
		bsp_frestore(f);

		if(len == 0)
		{
			if(timeout == 0)
			{
				ret = RAGAIN;
			}
			else
			{
				f = bsp_fsave();
				current->wakeup_cause = RERROR;
				if(timeout > 0)
				{
					assert(current->t_delay.param[0] == TASK_T(current));
					assert(!(current->flags & TASK_FLAGS_DELAYING));
					ptimer_start(&sysdelay_queue, &current->t_delay, timeout);
					current->flags |= TASK_FLAGS_DELAYING;
				}
				task_block(&(msgq->taskq));
				bsp_frestore(f);

				ret = RERROR;
				f = bsp_fsave();
				switch ((ret = current->wakeup_cause))
				{
				case ROK:
					len = mem_buffer_get(&msgq->buf, msg, *msglen);
					if(len <= 0)
						ret = RERROR;	/* double check for length */
					break;
					
				case RTIMEOUT:
				case RSIGNAL:
				default:
					break;
				}
				bsp_frestore(f);
			}
		}
		else
		{
			ret = ROK;
		}
		
		*msglen = len;
	}
	
	return ret;
}


