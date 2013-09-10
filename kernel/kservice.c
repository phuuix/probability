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
 *   kservice.c                             0.3.0
 *
 * COMPONENT
 *   Kernel
 *
 * DESCRIPTION
 *   privde kernel service such as CPU load statisstic
 *
 * CHANGELOG
 *   AUTHOR         DATE                    NOTES
 *   Bearix         2006-9-03               Version 0.3.0
 *************************************************************************/

#include <config.h>
#include <task.h>
#include <kservice.h>
#include <ipc.h>
#include <bsp.h>
#include <assert.h>

#include "probability.h"

#if 0
#include "stm32f2xx_gpio.h"
#include "stm32f2xx_rcc.h"
#endif

#define KSERV_MAIL_SIZE 2
#define KSERV_MAIL_NUM 1
#define KSERV_TIMEOUT	30

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
	short ret;
	uint32_t mail[2];
	uint32_t tick = 0;
	
	/* initialize mail box */
	ret = mbox_initialize(&kmbox, KSERV_MAIL_SIZE, KSERV_MAIL_NUM, NULL);
	assert(ret == ROK);

	runled_init();
	
	for(;;)
	{
		ret = mbox_pend(&kmbox, mail, KSERV_TIMEOUT);
		switch (ret)
		{
			case ROK:
				kprintf("kservice receives a mail\n");
				break;
			case RTIMEOUT:
				// LED control
				tick = ~tick;
				runled_toggle(tick);
				break;
			default:
				kprintf("kservice: warning unknown pend return: %d\n", ret);
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

