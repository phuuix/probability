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
    uint32_t tv_sec, tv_usec;
	struct journal_taskswitch *jevent = (struct journal_taskswitch *)JOURNAL_CLASS1_NEXTRECORD;
	
    bsp_gettime(&tv_sec, &tv_usec);
	jevent->usec = tv_usec;
	jevent->t_current = TASK_T(current);       // from must be current!
	jevent->jtype = JOURNAL_TYPE_TASKSWITCH;
	
	jevent->tfrom_state = from->state;
	jevent->tto_wakeup_cause = to->wakeup_cause;
	jevent->tto = TASK_T(to);
	jevent->tfrom_isdelaying = from->flags & TASK_FLAGS_DELAYING;
	jevent->is_interrupt = (sys_get_active_int()>0);
	jevent->filler = 0;

	JOURNAL_CLASS1_UPDATENEXT();
}

void journal_ipc_pend(ipc_t *ipc, int timeout)
{
	uint32_t tv_sec, tv_usec;
	struct journal_ipcpend *jevent = (struct journal_ipcpend *)JOURNAL_CLASS1_NEXTRECORD;

	bsp_gettime(&tv_sec, &tv_usec);
	jevent->usec = tv_usec;
	jevent->t_current = TASK_T(current);
	jevent->jtype = JOURNAL_TYPE_IPCPEND;

	jevent->subtype = ipc->type-1;
	jevent->t_ipc = ipc->t_parent;
	jevent->filler = 0;
	jevent->is_interrupt = (sys_get_active_int()>0);
	jevent->timeout = timeout;

	JOURNAL_CLASS1_UPDATENEXT();
}

void journal_ipc_post(ipc_t *ipc, dtask_t *task)
{
	uint32_t tv_sec, tv_usec;
	struct journal_ipcpost *jevent = (struct journal_ipcpost *)JOURNAL_CLASS1_NEXTRECORD;

	bsp_gettime(&tv_sec, &tv_usec);
	jevent->usec = tv_usec;
	jevent->t_current = TASK_T(current);
	jevent->jtype = JOURNAL_TYPE_IPCPOST;

	jevent->subtype = ipc->type-1;
	jevent->t_ipc = ipc->t_parent;
	jevent->t_wakeup = (task == NULL)?0:TASK_T(task);
	jevent->is_interrupt = (sys_get_active_int()>0);
	switch(ipc->type)
	{
		case IPC_TYPE_MBOX:
			jevent->ipcvalue = (((mbox_t *)ipc)->buf.len - ((mbox_t *)ipc)->buf.vlen);
			break;
		case IPC_TYPE_MSGQ:
			jevent->ipcvalue = (((msgq_t *)ipc)->buf.len - ((msgq_t *)ipc)->buf.vlen);
			break;
		case IPC_TYPE_SEMAPHORE:
			jevent->ipcvalue = ((sem_t *)ipc)->value;
			break;
		case IPC_TYPE_MUTEX:
			jevent->ipcvalue = ((mtx_t *)ipc)->value;
			break;
		default:
			jevent->ipcvalue = 0;
	}
	

	JOURNAL_CLASS1_UPDATENEXT();
}

/* record current time */
void journal_timestamp()
{
	uint32_t tv_sec, tv_usec;
	struct journal_time *jevent = (struct journal_time *)JOURNAL_CLASS1_NEXTRECORD;

	bsp_gettime(&tv_sec, &tv_usec);
	jevent->usec = tv_usec;
	jevent->t_current = TASK_T(current);
	jevent->jtype = JOURNAL_TYPE_TIMESTAMP;
	
    jevent->tv_sec = tv_sec;

    JOURNAL_CLASS1_UPDATENEXT();
}

void journal_interrupt(int type, int value)
{
	uint32_t tv_sec, tv_usec;
	struct journal_int *jevent = (struct journal_int *)JOURNAL_CLASS1_NEXTRECORD;

	bsp_gettime(&tv_sec, &tv_usec);
	jevent->usec = tv_usec;
	jevent->t_current = TASK_T(current);
	jevent->jtype = type;

	jevent->filler = 0;
	jevent->value = value;

    JOURNAL_CLASS1_UPDATENEXT();
}

void journal_user_defined(uint32_t event_type, uint32_t data)
{
    uint32_t tv_sec, tv_usec;
	struct journal_class1 *jevent = (struct journal_class1 *)JOURNAL_CLASS1_NEXTRECORD;

	bsp_gettime(&tv_sec, &tv_usec);
	jevent->usec = tv_usec;
	jevent->t_current = TASK_T(current);
	jevent->jtype = event_type;

    jevent->data = data;

    JOURNAL_CLASS1_UPDATENEXT();
}

/**************************************************************************
 * CLASS2 journal: object create and destroy
 **************************************************************************/
void journal_task_create(struct dtask *task)
{
	uint32_t tv_sec, tv_usec;
	struct journal_taskcreate *jevent = (struct journal_taskcreate *)JOURNAL_CLASS2_NEXTRECORD;

	bsp_gettime(&tv_sec, &tv_usec);
	jevent->usec = tv_usec;
	jevent->t_current = TASK_T(current);
	jevent->jtype = JOURNAL_TYPE_TASKCREATE;

	jevent->priority = task->priority;
	jevent->tid = TASK_T(task);
	jevent->is_sliced = !(task->flags & TASK_FLAGS_DONT_SLICE);
	jevent->is_static_stack = (task->flags & TASK_FLAGS_STATICSTACK);
	jevent->filler = 0;

	JOURNAL_CLASS2_UPDATENEXT();
}


void journal_task_exit(struct dtask *task)
{
    uint32_t tv_sec, tv_usec;
	struct journal_taskexit *jevent = (struct journal_taskexit *)JOURNAL_CLASS2_NEXTRECORD;

	bsp_gettime(&tv_sec, &tv_usec);
	jevent->usec = tv_usec;
	jevent->t_current = TASK_T(current);
	jevent->jtype = JOURNAL_TYPE_TASKEXIT;

	jevent->tid = TASK_T(task);
	jevent->tstate = task->state;
	jevent->tisdelaying = (task->flags & TASK_FLAGS_DELAYING);
	jevent->is_interrupt = (sys_get_active_int()>0);
	jevent->filler = 0;

	JOURNAL_CLASS2_UPDATENEXT();
}

void journal_ipc_init(ipc_t *ipc)
{
	uint32_t tv_sec, tv_usec;
	struct journal_ipcinit *jevent = (struct journal_ipcinit *)JOURNAL_CLASS2_NEXTRECORD;

	bsp_gettime(&tv_sec, &tv_usec);
	jevent->usec = tv_usec;
	jevent->t_current = TASK_T(current);
	jevent->jtype = JOURNAL_TYPE_IPCINIT;

	jevent->subtype = ipc->type-1;
	jevent->filler = 0;
	switch(ipc->type)
	{
	case IPC_TYPE_MBOX:
		jevent->value = ((mbox_t *)ipc)->buf.vlen;
		break;
	case IPC_TYPE_MSGQ:
		jevent->value = ((msgq_t *)ipc)->buf.vlen;
		break;
	case IPC_TYPE_SEMAPHORE:
		jevent->value = ((sem_t *)ipc)->value;
		break;
	case IPC_TYPE_MUTEX:
		jevent->value = ((sem_t *)ipc)->value;
		break;
	}
	
	JOURNAL_CLASS2_UPDATENEXT();
}

void journal_ipc_destroy(ipc_t *ipc)
{
	uint32_t tv_sec, tv_usec;
	struct journal_ipcdestroy *jevent = (struct journal_ipcdestroy *)JOURNAL_CLASS2_NEXTRECORD;

	bsp_gettime(&tv_sec, &tv_usec);
	jevent->usec = tv_usec;
	jevent->t_current = TASK_T(current);
	jevent->jtype = JOURNAL_TYPE_IPCDESTROY;

	jevent->subtype = ipc->type-1;
	jevent->is_wakeup = !DLLIST_EMPTY(&(ipc->taskq));
	jevent->is_freedata = ipc->flag & IPC_FLAG_FREEMEM;
	jevent->filler = 0;
	
	switch(ipc->type)
	{
	case IPC_TYPE_MBOX:
		jevent->value = ((mbox_t *)ipc)->buf.vlen;
		break;
	case IPC_TYPE_MSGQ:
		jevent->value = ((msgq_t *)ipc)->buf.vlen;
		break;
	case IPC_TYPE_SEMAPHORE:
		jevent->value = ((sem_t *)ipc)->value;
		break;
	case IPC_TYPE_MUTEX:
		jevent->value = ((sem_t *)ipc)->value;
		break;
	}
	
	JOURNAL_CLASS2_UPDATENEXT();
}

const char *journal_type_str[] = {"TS", "TM", "PT", "PD", "IE", "ID", "IT", "FI", "FO", "CC", "US"};
/* 
 * dump journal information 
 * We use kprintf instead of uprintf in journal_dump() because it will be called in crash dump
 */
void journal_dump()
{
	int idx;
	uint32_t *ptr;

	/* in this function we assume JOURNAL_CLASS1_MAXRECORD and JOURNAL_CLASS2_MAXRECORD is power of 2 */
	kprintf("\nClass1 journal (index=%d):\n", journal_class1_index);
	for(idx=0; idx<JOURNAL_CLASS1_MAXRECORD; idx+=4)
	{
		ptr = (uint32_t *)&journal_class1_history[idx];
		kprintf(" 0x%08x 0x%08x  0x%08x 0x%08x  0x%08x 0x%08x  0x%08x 0x%08x\n", ptr[0], ptr[1], ptr[2], ptr[3], ptr[4], ptr[5], ptr[6], ptr[7]);
	}

	kprintf("Class2 journal (index=%d):\n", journal_class2_index);
	for(idx=0; idx<JOURNAL_CLASS2_MAXRECORD; idx+=4)
	{
		ptr = (uint32_t *)&journal_class2_history[idx];
		kprintf(" 0x%08x 0x%08x  0x%08x 0x%08x  0x%08x 0x%08x  0x%08x 0x%08x\n", ptr[0], ptr[1], ptr[2], ptr[3], ptr[4], ptr[5], ptr[6], ptr[7]);
	}

	for(idx=0; idx<JOURNAL_CLASS1_MAXRECORD; idx++)
	{
		journal_class1_t *jptr;

		jptr = (journal_class1_t *)&journal_class1_history[idx];
		kprintf("us: %08d %8s %s data: %08x %08x\n", jptr->usec, systask[jptr->t_current].name, journal_type_str[jptr->jtype], 
				*(uint32_t *)&journal_class1_history[idx], jptr->data);
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

	/* for current task, simulate a task switch out and in */
	bsp_gettime(&time_sec, &time_ns);
	current->time_accumulate_c_ns += 1000000 /* now is us NOT ns */ - current->time_in_ns;
	current->time_in_sec = time_sec;
	current->time_in_ns = time_ns;

	/*kprintf("time_sec=%d time_ns=%d, sys_time_calibration=%d current=%s time_in_ns=%d time_accumulate_c_ns=%d\n", 
		time_sec, time_ns, sys_time_calibration, 
		current->name, current->time_in_ns, current->time_accumulate_c_ns);*/

	/* populate the CPU usage value (as us) for every active task */
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

