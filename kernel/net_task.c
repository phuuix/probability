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
/*   net_task.c                                    0.3.0                     */
/*                                                                       */
/* COMPONENT                                                             */
/*   Kernel                                                              */
/*                                                                       */
/* DESCRIPTION                                                           */
/*   Deal with network transactions                                      */
/*                                                                       */
/* CHANGELOG                                                             */
/*   AUTHOR         DATE                    NOTES                        */
/*   Bearix         2006-8-20               Version 0.3.0                */
/*************************************************************************/ 


#include <config.h>
#include <stdio.h>
#include <assert.h>
#include <task.h>
#include <ipc.h>
#include <net/net_task.h>
#include <bsp.h>

mbox_t net_mbox;
static uint8_t net_mbox_data[sizeof(struct net_msg)*200];
static char net_task_stack[0x1000];

int net_task_job_add(void (*func)(), void *arg)
{
	struct net_msg msg;

	msg.func = func;
	msg.arg = arg;

	return mbox_post(&net_mbox, (uint32_t *)&msg);
}

static void net_task(void *p)
{
	struct net_msg msg;

	mbox_initialize(&net_mbox, sizeof(struct net_msg)/sizeof(int), 200, net_mbox_data);

	while(1)
	{
		int ret;
		
		ret = mbox_pend(&net_mbox, (uint32_t *)&msg, TIME_WAIT_FOREVER);
		if(ret == ROK)
		{
			if(msg.func)
				msg.func(msg.arg);
		}
		else
		{
			kprintf("net task mbox_pend() error: ret=%d\n", ret);
		}
	}
}

/*
 * Create and start net task.
 */
void net_task_init()
{
	task_t t;

	t = task_create("tnet", net_task, NULL, net_task_stack, 0x1000, 0x60, 0, 0);
	assert(t != RERROR);
	task_resume_noschedule(t);
}

