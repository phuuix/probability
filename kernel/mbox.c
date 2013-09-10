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
 *   mbox.c                                    0.3.0
 *
 * COMPONENT
 *   Kernel
 *
 * DESCRIPTION
 *   Mail box is a variance of message queue, every message's size is 4
 *   long words.
 *
 * CHANGELOG
 *   AUTHOR         DATE                    NOTES
 *   Bearix         2006-8-20               Version 0.3.0
 *   Bearix         2013-1-28               Port to ARM Cortex-m, avoid unnessesary disable 
 *                                                    interrupt to adapt pendsv task switch
 *************************************************************************/ 

#include <config.h>

#include <task.h>
#include <ipc.h>
#include <assert.h>
#include <bsp.h>
#include <ctype.h>
#include <stdlib.h>

#include "probability.h"
#include "journal.h"

/*
 * initialize a mail box
 *   mbox: pointer to mail box
 *   usize: size of mail unit (in u32)
 *   unum: number of mails accommodated
 *   data: sizeof data must >= usize*unum*sizeof(uint32_t)
 *   return: ROK on success, RERROR on failed
 */
int mbox_initialize(mbox_t *mbox, uint16_t usize, uint16_t unum, void *data)
{
	uint32_t f;

	if(mbox == NULL)
		return RERROR;

	if(data == NULL)
	{
		/* TODO: set a flag to free data */
		data = malloc(usize*unum*sizeof(uint32_t));
	}
	
	if(data == NULL)
		return RERROR;

	f = bsp_fsave();
	
	mbox->type = IPC_TYPE_MBOX;
	mbox->flag = IPC_FLAG_VALID;
	blockq_init(&mbox->taskq);
	queue_buffer_init(&mbox->buf, data, usize, unum);
	
	bsp_frestore(f);

	return ROK;
}


/*
 * cleanup a mail box
 *   mbox: pointer to mail box
 */
void mbox_destroy(mbox_t *mbox)
{
	struct dtask *t;
	uint32_t f;

	if(mbox && (mbox->flag & IPC_FLAG_VALID))
	{
		f = bsp_fsave();
		mbox->flag = 0;

		/* wake up all tasks in mbox's block task queue */
		while((t = blockq_select(&mbox->taskq)))
		{
			task_wakeup_noschedule(TASK_T(t));
		}
		task_schedule();

		/* left mbox->buf.data to be freed by user */
		bsp_frestore(f);
	}
}


/*
 * send a message to mail box
 *   mbox: mail box
 *   msg: message
 *   return: RERROR on failed; ROK on success
 */
int mbox_post(mbox_t *mbox, uint32_t *msg)
{
	int ret = RERROR;
	uint32_t f;
	struct dtask *t;

	if(mbox && (mbox->flag & IPC_FLAG_VALID))
	{
		f = bsp_fsave();
		if(queue_buffer_put(&mbox->buf, msg))
		{
			ret = ROK;
			t = blockq_select(&(mbox->taskq));
			if(t)
			{
				journal_ipc_post((ipc_t *)mbox, TASK_T(t));
				t->wakeup_cause = ROK;
				task_wakeup(t-systask);
			}
		}
		bsp_frestore(f);
	}

	return ret;
}


/*
 * receive an message from mail box
 *   mbox: mail box
 *   msg: message buffer
 *   timeout: receive's max wait time
 *     <0 -- wait forever; 0 -- no wait; >0 -- wait for timeout ticks
 *   retrun: RERROR on failed, RTIMEOUT on timeout, RAGAIN on busy, ROK on success.
 */
int mbox_pend(mbox_t *mbox, uint32_t *msg, int timeout)
{
	uint32_t f, len;
	int ret = RERROR;
	struct dtask *myself;

	if(mbox && (mbox->flag & IPC_FLAG_VALID))
	{
		ret = ROK;
		f = bsp_fsave();
		len = queue_buffer_get(&mbox->buf, msg);
		bsp_frestore(f);
		if(len == 0)
		{
			if(timeout == 0)
				ret = RAGAIN;
			else
			{
				f = bsp_fsave();
				myself = current;
				current->wakeup_cause = RERROR;
				if(timeout > 0)
				{
					assert(current->t_delay.param[0] == TASK_T(current));
					assert(!(current->flags & TASK_FLAGS_DELAYING));
					ptimer_start(&sysdelay_queue, &current->t_delay, timeout);
					current->flags |= TASK_FLAGS_DELAYING;
				}

				journal_ipc_pend((ipc_t *)mbox, TASK_T(current));
				task_block(&(mbox->taskq));
				bsp_frestore(f);

				/* at this point, wakeup cause is modified */
				switch ((ret = myself->wakeup_cause))
				{
				case ROK:
					f = bsp_fsave();
					len = queue_buffer_get(&mbox->buf, msg);
					bsp_frestore(f);
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
	}
	
	return ret;
}


