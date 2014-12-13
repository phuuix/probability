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
 *   Deal with network transactions
 *
 * @History
 *   AUTHOR         DATE                 NOTES
 *   
 */

#include <config.h>
#include <stdio.h>
#include <assert.h>
#include <task.h>
#include <ipc.h>
#include <net/net_task.h>
#include <bsp.h>
#include "uprintf.h"

#define TNET_MBOX_SIZE 20

static mbox_t net_mbox;
static uint8_t net_mbox_data[sizeof(struct net_msg)*TNET_MBOX_SIZE];
static char net_task_stack[TNET_STACK_SIZE];

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
			uprintf(UPRINT_ERROR, UPRINT_BLK_NET, "net task mbox_pend() error: ret=%d\n", ret);
		}
	}
}

/*
 * Create and start net task.
 */
void net_task_init()
{
	task_t t;

	mbox_initialize(&net_mbox, sizeof(struct net_msg)/sizeof(int), TNET_MBOX_SIZE, net_mbox_data);
	
	t = task_create("tnet", net_task, NULL, net_task_stack, TNET_STACK_SIZE, TNET_PRIORITY, 20, 0);
	assert(t != RERROR);
	task_resume_noschedule(t);
}

