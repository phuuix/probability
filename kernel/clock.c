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
/*   clock.c                                   0.3.0                     */
/*                                                                       */
/* COMPONENT                                                             */
/*   Kernel                                                              */
/*                                                                       */
/* DESCRIPTION                                                           */
/*                                                                       */
/*                                                                       */
/* CHANGELOG                                                             */
/*   AUTHOR         DATE                    NOTES                        */
/*   Bearix         2006-8-20               Version 0.3.0                */
/*************************************************************************/ 

#include <config.h>
#include <inttypes.h>
#include <clock.h>
#include <list.h>
#include <task.h>
#include <bsp.h>
#include <ptimer.h>
#include "assert.h"

uint32_t sys_passed_ticks;
uint32_t sys_passed_secs;


static void check_delay()
{
	ptimer_consume_time(&sysdelay_queue, 1);
}


static void timer_handler()
{
	static uint16_t ticks = 0;
	uint32_t f;
	struct dtask *task;

	/* we have to disable interrupt for stm32's nest interrupt */
	f = bsp_fsave();
	
	/* update time */
	sys_passed_ticks++;
	ticks++;
	if(ticks > TICKS_PER_SECOND)
	{
		sys_passed_secs ++;
		ticks -= TICKS_PER_SECOND;
	}

	/* check delay every one second */
	check_delay();

	/* dealwith timeslice */
	task = current;
	if((task->flags & TASK_FLAGS_DONT_SLICE) == 0)
	{
		task->timeslice_left--;
		if(task->timeslice_left == 0)
		{
			/* yield CPU */
			task->timeslice_left = task->timeslice_all;
			
			assert(task->state == TASK_STATE_READY)
			readyq_dequeue(&sysready_queue, task);
			readyq_enqueue(&sysready_queue, task);
		}
	}
	task_schedule();
	
	bsp_frestore(f);
}


/*
 * get passed seconds from boot
 */
uint32_t time(uint32_t *t)
{
	uint32_t tt;

	tt = sys_passed_secs;
	if(t)
		*t = tt;

	return tt;
}

uint32_t tick()
{
	return sys_passed_ticks;
}

void clock_init(int irq)
{
	sys_passed_ticks = 0;
	sys_passed_secs = 0;

	bsp_isr_attach(irq, timer_handler);
	bsp_irq_unmask(irq);
}
