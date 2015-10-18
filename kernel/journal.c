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
 *   system journal  and performance counter
 *
 * @History
 *   AUTHOR         DATE           NOTES
 */

#include <string.h>
#include <assert.h>

#include "bsp.h"
#include "memory.h"
#include "task.h"
#include "clock.h"
#include "journal.h"
#include "uprintf.h"

#ifdef INCLUDE_JOURNAL

#define JOURNAL_CLASS1_NEXTRECORD (&journal_class1_history[journal_class1_index]);
#define JOURNAL_CLASS1_UPDATENEXT() \
	do{ \
		journal_class1_index = ((journal_class1_index+1)>=JOURNAL_CLASS1_MAXRECORD)?0:(journal_class1_index+1); \
	}while(0)

#define JOURNAL_CLASS2_NEXTRECORD (&journal_class2_history[journal_class2_index]);
#define JOURNAL_CLASS2_UPDATENEXT() \
	do{ \
		journal_class2_index = ((journal_class2_index+1)>=JOURNAL_CLASS2_MAXRECORD)?0:(journal_class2_index+1); \
	}while(0)

static journal_class1_t journal_class1_history[JOURNAL_CLASS1_MAXRECORD];
static journal_class2_t journal_class2_history[JOURNAL_CLASS2_MAXRECORD];
static uint16_t journal_class1_index;
static uint16_t journal_class2_index;


void journal_memory_alloc(uint32_t size, void *data, char *file, uint32_t line, task_t task)
{
	uprintf(UPRINT_INFO, UPRINT_BLK_MEM, "Alloc: size=%d data=0x%x task=%d\n", size, data, task);

	// TODO:  save data block's attribute
}

void journal_memory_free(uint32_t size, void *data, char *file, uint32_t line, task_t task)
{
	uprintf(UPRINT_INFO, UPRINT_BLK_MEM, "Free: size=%d data=0x%x task=%d\n", size, data, task);

	// TODO:  save data block's attribute
}

/**************************************************************************
 * CLASS1 journal: object action
 **************************************************************************/
void journal_task_switch(struct dtask *from, struct dtask *to)
{
    uint32_t tv_sec;

	// save task switch history
	struct journal_taskswitch *jtaskswitch = (struct journal_taskswitch *)JOURNAL_CLASS1_NEXTRECORD;
    bsp_gettime(&tv_sec, &jtaskswitch->time);
	jtaskswitch->jtype = JOURNAL_TYPE_TASKSWITCH;
	jtaskswitch->t_from = TASK_T(from);
	jtaskswitch->t_to = TASK_T(to);
	jtaskswitch->tfrom_state = from->state;
	jtaskswitch->tfrom_flags = from->flags | (sys_get_active_int()<<4);
	jtaskswitch->tto_wakeup_cause = to->wakeup_cause;

	JOURNAL_CLASS1_UPDATENEXT();
}

void journal_ipc_pend(ipc_t *ipc, task_t task)
{
    uint32_t tv_sec;

	// save ipc history
	struct journal_ipcop *jipcop = (struct journal_ipcop *)JOURNAL_CLASS1_NEXTRECORD;
	bsp_gettime(&tv_sec, &jipcop->time);
	jipcop->ipc_ptr = ipc;
	jipcop->ipctype = ipc->type;
    jipcop->active_ints = sys_get_active_int();
	jipcop->tid = TASK_T(current);
	jipcop->jtype = JOURNAL_TYPE_IPCPEND;

	JOURNAL_CLASS1_UPDATENEXT();
}


void journal_ipc_post(ipc_t *ipc, task_t task)
{
    uint32_t tv_sec;

	// save ipc history
	struct journal_ipcop *jipcop = (struct journal_ipcop *)JOURNAL_CLASS1_NEXTRECORD;
	bsp_gettime(&tv_sec, &jipcop->time);
	jipcop->ipc_ptr = ipc;
	jipcop->ipctype = ipc->type;
    jipcop->active_ints = sys_get_active_int();
	jipcop->tid = TASK_T(current);
	jipcop->jtype = JOURNAL_TYPE_IPCPOST;

	JOURNAL_CLASS1_UPDATENEXT();
}

/* record current time */
void journal_timestamp()
{
    struct journal_time *jtime;

    jtime = (struct journal_time *)JOURNAL_CLASS1_NEXTRECORD;
    bsp_gettime(&jtime->tv_sec, &jtime->time);
    jtime->filler = JOURNAL_TYPE_TIMESTAMP;
    jtime->cur_tid = TASK_T(current);
    jtime->jtype = JOURNAL_TYPE_TIMESTAMP;

    JOURNAL_CLASS1_UPDATENEXT();
}

void journal_user_defined(uint32_t data1, uint32_t data2)
{
    uint32_t flags;
    uint32_t tv_sec;
    uint32_t *pdata;

    flags = bsp_fsave();

    pdata = (uint32_t *)JOURNAL_CLASS1_NEXTRECORD;
    bsp_gettime(&tv_sec, &pdata[0]);
    pdata[1] = data1;
    pdata[2] = data2;

    JOURNAL_CLASS1_UPDATENEXT();

    bsp_frestore(flags);
}

/**************************************************************************
 * CLASS2 journal: object create and destroy
 **************************************************************************/
void journal_task_create(struct dtask *task)
{
    uint32_t tv_sec;

	struct journal_tasklife *jtasklife = (struct journal_tasklife *)JOURNAL_CLASS2_NEXTRECORD;
	bsp_gettime(&tv_sec, &jtasklife->time);
	jtasklife->jtype = JOURNAL_TYPE_TASKCREATE;
	jtasklife->flags = task->flags;
	jtasklife->filler = 0xFFFF;
	jtasklife->tid = TASK_T(task);
	jtasklife->t_parent = TASK_T(current);

	JOURNAL_CLASS2_UPDATENEXT();
}


void journal_task_exit(struct dtask *task)
{
    uint32_t tv_sec;

	struct journal_tasklife *jtasklife = (struct journal_tasklife *)JOURNAL_CLASS2_NEXTRECORD;
	bsp_gettime(&tv_sec, &jtasklife->time);
	jtasklife->jtype = JOURNAL_TYPE_TASKEXIT;
	jtasklife->flags = task->flags;
	jtasklife->filler = 0xFFFF;
	jtasklife->tid = TASK_T(task);
	jtasklife->t_parent = TASK_T(current);

	JOURNAL_CLASS2_UPDATENEXT();
}

void journal_ipc_init(ipc_t *ipc)
{
    uint32_t tv_sec;

	struct journal_ipclife *jipclife = (struct journal_ipclife *)JOURNAL_CLASS2_NEXTRECORD;
	bsp_gettime(&tv_sec, &jipclife->time);
	jipclife->tid = TASK_T(current);
	jipclife->jtype = JOURNAL_TYPE_IPCINIT;
	jipclife->ipctype = ipc->type;
	jipclife->ipc_ptr = ipc;

	JOURNAL_CLASS2_UPDATENEXT();
}

void journal_ipc_destroy(ipc_t *ipc)
{
    uint32_t tv_sec;

	struct journal_ipclife *jipclife = (struct journal_ipclife *)JOURNAL_CLASS2_NEXTRECORD;
	bsp_gettime(&tv_sec, &jipclife->time);
	jipclife->tid = TASK_T(current);
	jipclife->jtype = JOURNAL_TYPE_IPCDESTROY;
	jipclife->ipctype = ipc->type;
	jipclife->ipc_ptr = ipc;

	JOURNAL_CLASS2_UPDATENEXT();
}

/* 
 * dump journal information 
 * We use kprintf instead of uprintf in journal_dump() because it will be called in crash dump
 */
void journal_dump()
{
	int idx;
	uint32_t *ptr1, *ptr2;

	/* in this function we assume JOURNAL_CLASS1_MAXRECORD and JOURNAL_CLASS2_MAXRECORD is power of 2 */
	kprintf("\nClass1 journal (index=%d):\n", journal_class1_index);
	for(idx=0; idx<JOURNAL_CLASS1_MAXRECORD; idx+=2)
	{
		ptr1 = (uint32_t *)&journal_class1_history[idx];
		ptr2 = (uint32_t *)&journal_class1_history[idx+1];
		kprintf(" 0x%08x 0x%08x 0x%08x    0x%08x 0x%08x 0x%08x\n", ptr1[0], ptr1[1], ptr1[2], ptr2[0], ptr2[1], ptr2[2]);
	}

	kprintf("Class2 journal (index=%d):\n", journal_class2_index);
	for(idx=0; idx<JOURNAL_CLASS2_MAXRECORD; idx+=2)
	{
		ptr1 = (uint32_t *)&journal_class2_history[idx];
		ptr2 = (uint32_t *)&journal_class2_history[idx+1];
		kprintf(" 0x%08x 0x%08x 0x%08x    0x%08x 0x%08x 0x%08x\n", ptr1[0], ptr1[1], ptr1[2], ptr2[0], ptr2[1], ptr2[2]);
	}
}
#endif // INCLUDE_JOURNAL

/**************************************************************************
 * System level counterl
 **************************************************************************/
#ifdef INCLUDE_PMCOUNTER
uint32_t PMC_sys32_counter[PMC_U32_nSysmax];
/* update PMC CPU usage for every 1s */
void pmc_calc_cpu_usage()
{
	uint32_t time_sec, time_ns, t;
	dtask_t *task;

	/* for current task, simulate on swith out and in */
	bsp_gettime(&time_sec, &time_ns);
	current->time_accumulate_c_ns += 1000000000 - current->time_in_ns;
	current->time_in_sec = time_sec;
	current->time_in_ns = time_ns;

	/*kprintf("time_sec=%d time_ns=%d, sys_time_calibration=%d current=%s time_in_ns=%d time_accumulate_c_ns=%d\n", 
		time_sec, time_ns, sys_time_calibration, 
		current->name, current->time_in_ns, current->time_accumulate_c_ns);*/
	
	for(t=1; t<MAX_TASK_NUMBER; t++)
	{
		task = &systask[t];
		if(task->state != TASK_STATE_DEAD)
		{
			PMC_SET_COUNTER(task->PMC_task32[PMC_U32_Task_nsConsumedInLastSecond], task->time_accumulate_c_ns);
			task->time_accumulate_c_ns = 0;
		}
	}
}

void pmc_dump()
{
	uint32_t i;

	kprintf("System level counters (total %d):\n", PMC_U32_nSysmax);
	
	i=0;
	while(i<PMC_U32_nSysmax)
	{
		kprintf(" %08d", PMC_sys32_counter[i]);
		i++;
		if(!(i & (8-1))) 
			kprintf("\n");
	}
	kprintf("\n");

	for(i=1; i<MAX_TASK_NUMBER; i++)
	{
		if(systask[i].state != TASK_STATE_DEAD)
			kprintf("Task %d (%8s) counters: %08d %08d %08d\n", i, systask[i].name,
				systask[i].PMC_task32[0]/1000, systask[i].PMC_task32[1], systask[i].PMC_task32[2]);
	}
}

#endif // INCLUDE_PMCOUNTER

