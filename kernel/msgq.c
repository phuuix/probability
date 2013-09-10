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
 *   msgq.c                                    0.3.0
 *
 * COMPONENT
 *   Kernel
 *
 * DESCRIPTION
 *   message queue implement
 *
 * CHANGELOG
 *   AUTHOR         DATE                    NOTES
 *   Bearix         2006-8-20               Version 0.3.0
 *************************************************************************/ 


#include <config.h>

#include <task.h>
#include <ipc.h>
#include <assert.h>
#include <memory.h>
#include <bsp.h>
#include <ctype.h>
#include <stdlib.h>
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
		ret = ROK;
		f = bsp_fsave();
		len = mem_buffer_get(&msgq->buf, msg, *msglen);

		if(len == 0)
		{
			if(timeout == 0)
				ret = RAGAIN;
			else
			{
				current->wakeup_cause = RERROR;
				if(timeout > 0)
				{
					assert(current->t_delay.param[0] == TASK_T(current));
					assert(!(current->flags & TASK_FLAGS_DELAYING));
					ptimer_start(&sysdelay_queue, &current->t_delay, timeout);
					current->flags |= TASK_FLAGS_DELAYING;
				}
				task_block(&(msgq->taskq));

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
			}
		}
		bsp_frestore(f);
		
		*msglen = len;
	}
	
	return ret;
}


