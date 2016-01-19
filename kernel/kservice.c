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
 *   provide kernel service as requested period, priority and function point
 *   provide timer service
 *
 * @History
 *   AUTHOR         DATE           NOTES
 */

#include <config.h>
#include <task.h>
#include <kservice.h>
#include <ipc.h>
#include <bsp.h>
#include <assert.h>

#include "uprintf.h"
#include "ptimer.h"
#include "probability.h"
#include "clock.h"

#define KSERV_MAIL_NUM 2
#define KSERV_TIMEOUT  2
#define KSERV_TIMESLOT_NUM  8
#define KSERV_TIMER_NUM 4

static struct mailbox kmbox;

void runled_init()
{
#if 0
	GPIO_InitTypeDef GPIO_InitStructure;

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOE, ENABLE);
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_25MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL ;
	GPIO_Init(GPIOE, &GPIO_InitStructure);
#endif
}

void runled_toggle(uint32_t tick)
{
#if 0
	if(tick)
		GPIO_SetBits(GPIOE, GPIO_Pin_1);
	else
		GPIO_ResetBits(GPIOE, GPIO_Pin_1);
#endif
}

/*
 * kservice task
 */
static void kserv_task(void *p)
{
	int ret;
	uint32_t now_tick, last_tick;
	kserv_mail_t mail;
	ptimer_table_t ktime_table;
	dllist_node_t ktime_slots[KSERV_TIMESLOT_NUM];
    ptimer_t ktime_timer[KSERV_TIMER_NUM];
	ptimer_t * timer;

	/* init mbox first */
	assert(ROK == mbox_initialize(&kmbox, sizeof(kserv_mail_t)/sizeof(uint32_t), KSERV_MAIL_NUM, NULL));
	
	ptimer_init_nomalloc(&ktime_table, KSERV_TIMESLOT_NUM, ktime_slots);

	last_tick = tick();
	for(;;)
	{
		ret = mbox_pend(&kmbox, (uint32_t *)&mail, KSERV_TIMEOUT);
		switch (ret)
		{
			case ROK:
                if(mail.func)
                {
                    int i;
                    
                    /* find a free timer */
                    timer=NULL;
                    for(i=0; i<KSERV_TIMER_NUM; i++)
                    {
                        if(!ptimer_is_running(&ktime_timer[i]))
                            timer = &ktime_timer[i];
                    }

                    if(timer)
                    {
                        timer->onexpired_func = mail.func;
                        timer->param[0] = mail.param[0];
                        timer->param[1] = mail.param[1];
                        timer->flags = mail.periodic*2;
                        ptimer_start(&ktime_table, timer, mail.interval);
                    }
                }
			case RTIMEOUT:
				now_tick = tick();
				ptimer_consume_time(&ktime_table, now_tick-last_tick);
				last_tick = now_tick;
				break;
			default:
				uprintf(UPRINT_WARNING, UPRINT_BLK_DEF, "kservice: warning unknown pend return: %d\n", ret);
				break;
		}
	}
}


/*
 * send a mail to kservice task
 *    type:
 *    data:
 *    return: ROK on successful
 */
int kserv_request(int type, int data)
{
	return ROK;
}


/* start kerservice task */
void kservice_init()
{
	task_t tt;
	
	/* start kservice task */
	tt = task_create("tkserv", kserv_task, NULL, NULL, TKSERV_STACK_SIZE, TKSERV_PRIORITY, 0, 0);
	task_resume_noschedule(tt);
}

