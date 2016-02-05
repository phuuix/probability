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
	jevent->usec = tv_usec/1000;
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

void journal_mbox_pend(mbox_t *mbox, int timeout)
{
	uint32_t tv_sec, tv_usec;
	struct journal_mboxpend *jevent = (struct journal_mboxpend *)JOURNAL_CLASS1_NEXTRECORD;

	bsp_gettime(&tv_sec, &tv_usec);
	jevent->usec = tv_usec/1000;
	jevent->t_current = TASK_T(current);
	jevent->jtype = JOURNAL_TYPE_MBOXPEND;

	jevent->t_ipc = mbox->t_parent;
	jevent->filler = 0;
	jevent->is_interrupt = (sys_get_active_int()>0);
	jevent->timeout = timeout;

	JOURNAL_CLASS1_UPDATENEXT();
}

void journal_mbox_post(mbox_t *mbox, dtask_t *task)
{
	uint32_t tv_sec, tv_usec;
	struct journal_mboxpost *jevent = (struct journal_mboxpost *)JOURNAL_CLASS1_NEXTRECORD;

	bsp_gettime(&tv_sec, &tv_usec);
	jevent->usec = tv_usec/1000;
	jevent->t_current = TASK_T(current);
	jevent->jtype = JOURNAL_TYPE_MBOXPOST;

	jevent->t_ipc = mbox->t_parent;
	jevent->t_wakeup = (task == NULL)?0:TASK_T(task);
	jevent->is_interrupt = (sys_get_active_int()>0);
	jevent->is_wakeup = (task != NULL);
	jevent->filler = 0;

	JOURNAL_CLASS1_UPDATENEXT();
}

void journal_sem_pend(sem_t *sem, int timeout)
{
	uint32_t tv_sec, tv_usec;
	struct journal_sempend *jevent = (struct journal_sempend *)JOURNAL_CLASS1_NEXTRECORD;

	bsp_gettime(&tv_sec, &tv_usec);
	jevent->usec = tv_usec/1000;
	jevent->t_current = TASK_T(current);
	jevent->jtype = JOURNAL_TYPE_SEMPEND;

	jevent->t_ipc = sem->t_parent;
	jevent->semvalue = sem->value;
	jevent->is_interrupt = (sys_get_active_int()>0);
	jevent->timeout = timeout;

	JOURNAL_CLASS1_UPDATENEXT();
}

void journal_sem_post(sem_t *sem, dtask_t *task)
{
    uint32_t tv_sec, tv_usec;
	struct journal_sempost *jevent = (struct journal_sempost *)JOURNAL_CLASS1_NEXTRECORD;

	bsp_gettime(&tv_sec, &tv_usec);
	jevent->usec = tv_usec/1000;
	jevent->t_current = TASK_T(current);
	jevent->jtype = JOURNAL_TYPE_SEMPOST;
	
	jevent->t_ipc = sem->t_parent;
	jevent->t_wakeup = (task == NULL)?0:TASK_T(task);
	jevent->semvalue = sem->value;
	jevent->is_interrupt = (sys_get_active_int()>0);
	jevent->is_wakeup = (task != NULL);
	jevent->filler = 0;

	JOURNAL_CLASS1_UPDATENEXT();
}

/* record current time */
void journal_timestamp()
{
	uint32_t tv_sec, tv_usec;
	struct journal_time *jevent = (struct journal_time *)JOURNAL_CLASS1_NEXTRECORD;

	bsp_gettime(&tv_sec, &tv_usec);
	jevent->usec = tv_usec/1000;
	jevent->t_current = TASK_T(current);
	jevent->jtype = JOURNAL_TYPE_TIMESTAMP;
	
    jevent->tv_sec = tv_sec;

    JOURNAL_CLASS1_UPDATENEXT();
}

void journal_user_defined(uint32_t event_type, uint32_t data)
{
	uint32_t flags;
    uint32_t tv_sec, tv_usec;
	struct journal_class1 *jevent = (struct journal_class1 *)JOURNAL_CLASS1_NEXTRECORD;

	flags = bsp_fsave();
	bsp_gettime(&tv_sec, &tv_usec);
	jevent->usec = tv_usec/1000;
	jevent->t_current = TASK_T(current);
	jevent->jtype = event_type;

    jevent->data = data;

    JOURNAL_CLASS1_UPDATENEXT();

    bsp_frestore(flags);
}

/**************************************************************************
 * CLASS2 journal: object create and destroy
 **************************************************************************/
void journal_task_create(struct dtask *task)
{
	uint32_t tv_sec, tv_usec;
	struct journal_taskcreate *jevent = (struct journal_taskcreate *)JOURNAL_CLASS2_NEXTRECORD;

	bsp_gettime(&tv_sec, &tv_usec);
	jevent->usec = tv_usec/1000;
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
	jevent->usec = tv_usec/1000;
	jevent->t_current = TASK_T(current);
	jevent->jtype = JOURNAL_TYPE_TASKEXIT;

	jevent->tid = TASK_T(task);
	jevent->tstate = task->state;
	jevent->tisdelaying = (task->flags & TASK_FLAGS_DELAYING);
	jevent->is_interrupt = (sys_get_active_int()>0);
	jevent->filler = 0;

	JOURNAL_CLASS2_UPDATENEXT();
}

void journal_mbox_init(mbox_t *mbox, uint16_t usize, uint16_t unum)
{
	uint32_t tv_sec, tv_usec;
	struct journal_mboxinit *jevent = (struct journal_mboxinit *)JOURNAL_CLASS2_NEXTRECORD;

	bsp_gettime(&tv_sec, &tv_usec);
	jevent->usec = tv_usec/1000;
	jevent->t_current = TASK_T(current);
	jevent->jtype = JOURNAL_TYPE_MBOXINIT;

	jevent->usize = usize;
	jevent->unum = unum;
	
	JOURNAL_CLASS2_UPDATENEXT();
}

void journal_mbox_destroy(mbox_t *mbox)
{
	uint32_t tv_sec, tv_usec;
	struct journal_mboxdestroy *jevent = (struct journal_mboxdestroy *)JOURNAL_CLASS2_NEXTRECORD;

	bsp_gettime(&tv_sec, &tv_usec);
	jevent->usec = tv_usec/1000;
	jevent->t_current = TASK_T(current);
	jevent->jtype = JOURNAL_TYPE_MBOXDESTROY;

	jevent->is_wakeup = !DLLIST_EMPTY(&(mbox->taskq));
	jevent->is_freedata = mbox->flag & IPC_FLAG_FREEMEM;
	jevent->filler = 0;
	
	JOURNAL_CLASS2_UPDATENEXT();
}


void journal_sem_init(sem_t *sem)
{
    uint32_t tv_sec, tv_usec;
	struct journal_seminit *jevent = (struct journal_seminit *)JOURNAL_CLASS2_NEXTRECORD;

	bsp_gettime(&tv_sec, &tv_usec);
	jevent->usec = tv_usec/1000;
	jevent->t_current = TASK_T(current);
	jevent->jtype = JOURNAL_TYPE_SEMINIT;

	jevent->value = sem->value;
	
	JOURNAL_CLASS2_UPDATENEXT();
}

void journal_sem_destroy(sem_t *sem)
{
	uint32_t tv_sec, tv_usec;
	struct journal_semdestroy *jevent = (struct journal_semdestroy *)JOURNAL_CLASS2_NEXTRECORD;

	bsp_gettime(&tv_sec, &tv_usec);
	jevent->usec = tv_usec/1000;
	jevent->t_current = TASK_T(current);
	jevent->jtype = JOURNAL_TYPE_SEMDESTROY;

	jevent->is_wakeup = (sem->tasks);
	jevent->value = sem->value;
	jevent->filler = 0;
	
	JOURNAL_CLASS2_UPDATENEXT();
}

void journal_mtx_init(mtx_t *mtx)
{
    uint32_t tv_sec, tv_usec;
	struct journal_mtxinit *jevent = (struct journal_mtxinit *)JOURNAL_CLASS2_NEXTRECORD;

	bsp_gettime(&tv_sec, &tv_usec);
	jevent->usec = tv_usec/1000;
	jevent->t_current = TASK_T(current);
	jevent->jtype = JOURNAL_TYPE_MTXINIT;

	jevent->value = mtx->value;
	jevent->filler = 0;
	
	JOURNAL_CLASS2_UPDATENEXT();
}

void journal_mtx_destroy(mtx_t *mtx)
{
	uint32_t tv_sec, tv_usec;
	struct journal_mtxdestroy *jevent = (struct journal_mtxdestroy *)JOURNAL_CLASS2_NEXTRECORD;

	bsp_gettime(&tv_sec, &tv_usec);
	jevent->usec = tv_usec/1000;
	jevent->t_current = TASK_T(current);
	jevent->jtype = JOURNAL_TYPE_MTXDESTROY;

	jevent->is_wakeup = (mtx->tasks);
	jevent->holdtimes = mtx->holdtimes;
	jevent->value = mtx->value;
	
	JOURNAL_CLASS2_UPDATENEXT();
}

void journal_msgq_init(msgq_t *msgq, uint16_t size)
{
    uint32_t tv_sec, tv_usec;
	struct journal_msgqinit *jevent = (struct journal_msgqinit *)JOURNAL_CLASS2_NEXTRECORD;

	bsp_gettime(&tv_sec, &tv_usec);
	jevent->usec = tv_usec/1000;
	jevent->t_current = TASK_T(current);
	jevent->jtype = JOURNAL_TYPE_MSGQINIT;

	jevent->size = size;
	jevent->filler = 0;
	
	JOURNAL_CLASS2_UPDATENEXT();
}

void journal_msgq_destroy(msgq_t *msgq)
{
	uint32_t tv_sec, tv_usec;
	struct journal_msgqdestroy *jevent = (struct journal_msgqdestroy *)JOURNAL_CLASS2_NEXTRECORD;

	bsp_gettime(&tv_sec, &tv_usec);
	jevent->usec = tv_usec/1000;
	jevent->t_current = TASK_T(current);
	jevent->jtype = JOURNAL_TYPE_MSGQDESTROY;

	jevent->is_wakeup = !DLLIST_EMPTY(&(msgq->taskq));
	jevent->value = 0;
	jevent->filler = 0;
	
	JOURNAL_CLASS2_UPDATENEXT();
}

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

