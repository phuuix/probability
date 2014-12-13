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
 *   time APIs
 *
 * @History
 *   AUTHOR         DATE           NOTES
 */

#include <config.h>
#include <inttypes.h>
#include <clock.h>
#include <list.h>
#include <task.h>
#include <bsp.h>
#include <ptimer.h>
#include <assert.h>
#include "clock.h"
#include "journal.h"

uint32_t sys_passed_ticks;
uint32_t sys_passed_secs;
uint32_t sys_time_calibration;

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
	
	/* update timing
	 *  Warning: if interrupt is disabled over 10ms, then we will loss tick and cause some time (sync) issues.
	 */
	sys_passed_ticks++;
	ticks++;

	if(ticks > TICKS_PER_SECOND)
	{
		/* may required time calibration for some bsp */
		//bsp_time_calibration();
		if(sys_time_calibration == 0)
		{
			uint32_t s, ns;
			bsp_gettime(&s, &ns);
			sys_time_calibration = ns;
		}
		
		sys_passed_secs ++;
		ticks -= TICKS_PER_SECOND;
#ifdef INCLUDE_PMCOUNTER
		/* update PMC CPU usage for every 1s */
		pmc_calc_cpu_usage();
#endif
	}

#ifdef INCLUDE_PMCOUNTER
	/* update PMC_U32_nInterruptInCurrentTick and PMC_U32_nInterruptInLastTick */
	PMC_SET_COUNTER(PMC_sys32_counter[PMC_U32_nInterruptInLastTick], PMC_sys32_counter[PMC_U32_nInterruptInCurrentTick]);
	PMC_SET_COUNTER(PMC_sys32_counter[PMC_U32_nInterruptInCurrentTick],0);
#endif


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
