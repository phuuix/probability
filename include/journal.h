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
 *   Journal and PMC (performance counter)
 *
 * @History
 *   AUTHOR         DATE           NOTES
 *   puhuix         2016-01-08     refine journal history, two uint32_t per event
 */

#ifndef _JOURNAL_H_
#define _JOURNAL_H_

#ifndef CRASHDUMP  // crash dump is a host tool
#include "ipc.h"
#endif

/**********************************************************************
 * Journal
 * 
 **********************************************************************/
#define JOURNAL_CLASS1_MAXRECORD  128
#define JOURNAL_CLASS2_MAXRECORD  16

enum journal_class1_type
{
	JOURNAL_TYPE_TASKSWITCH,
	JOURNAL_TYPE_TIMESTAMP,
	JOURNAL_TYPE_MBOXPOST,
	JOURNAL_TYPE_MBOXPEND,
	JOURNAL_TYPE_SEMPOST,
	JOURNAL_TYPE_SEMPEND,
	JOURNAL_TYPE_MTXPOST,
	JOURNAL_TYPE_MTXPEND,
	JOURNAL_TYPE_MSGQPOST,
	JOURNAL_TYPE_MSGQPEND,  // 9
	JOURNAL_TYPE_CLASS1MAX
};

enum journal_class2_type
{
	JOURNAL_TYPE_TASKCREATE,
	JOURNAL_TYPE_TASKEXIT,
	JOURNAL_TYPE_MBOXINIT,
	JOURNAL_TYPE_MBOXDESTROY,
	JOURNAL_TYPE_SEMINIT,
	JOURNAL_TYPE_SEMDESTROY,
	JOURNAL_TYPE_MTXINIT,
	JOURNAL_TYPE_MTXDESTROY,
	JOURNAL_TYPE_MSGQINIT,
	JOURNAL_TYPE_MSGQDESTROY,
	JOURNAL_TYPE_CLASS2MAX
};

struct journal_taskswitch
{
	uint32_t usec:20;           // 20 bits can hold 1 second duration
	uint32_t t_current:8;
	uint32_t jtype:4;

	uint32_t tfrom_state:4;
	uint32_t tto_wakeup_cause:4;
	uint32_t tto:8;
	uint32_t tfrom_isdelaying:1;
	uint32_t is_interrupt:1;
	uint32_t filler:14;
};

struct journal_mboxpost
{
	uint32_t usec:20;           // 20 bits can hold 1 second duration
	uint32_t t_current:8;
	uint32_t jtype:4;

	uint32_t t_ipc:8;        	// parent of ipc
	uint32_t t_wakeup:8;        // task to wakeup (valid if is_wakeup==true)
	uint32_t is_interrupt:1;
	uint32_t is_wakeup:1;
	uint32_t filler:14;
};

struct journal_mboxpend
{
	uint32_t usec:20;           // 20 bits can hold 1 second duration
	uint32_t t_current:8;
	uint32_t jtype:4;

	uint32_t t_ipc:8;           // parent of ipc
	uint32_t filler:8;       
	uint32_t is_interrupt:1;
	uint32_t timeout:15;
};

struct journal_sempost
{
	uint32_t usec:20;           // 20 bits can hold 1 second duration
	uint32_t t_current:8;
	uint32_t jtype:4;

	uint32_t t_ipc:8;           // parent of ipc
	uint32_t t_wakeup:8;        // task to wakeup (valid if is_wakeup==true)
	uint32_t semvalue:8;
	uint32_t is_interrupt:1;
	uint32_t is_wakeup:1;
	uint32_t filler:6;
};

struct journal_sempend
{
	uint32_t usec:20;           // 20 bits can hold 1 second duration
	uint32_t t_current:8;
	uint32_t jtype:4;

	uint32_t t_ipc:8;           // parent of ipc
	uint32_t semvalue:8;       
	uint32_t is_interrupt:1;
	uint32_t timeout:15;
};


/* record the time event for every one second */
struct journal_time
{
    uint32_t usec:20;           // 20 bits can hold 1 second duration
	uint32_t t_current:8;
	uint32_t jtype:4;
	
    uint32_t tv_sec;            // second
};

typedef struct journal_class1
{
	uint32_t usec:20;           // 20 bits can hold 1 second duration in us
	uint32_t t_current:8;
	uint32_t jtype:4;

	uint32_t data;
}journal_class1_t;

struct journal_taskcreate
{
	uint32_t usec:20;           // 20 bits can hold 1 second duration
	uint32_t t_current:8;
	uint32_t jtype:4;

	uint32_t priority:8;
	uint32_t tid:8;             // the created task id
	uint32_t is_sliced:1;
	uint32_t is_static_stack:1;
	uint32_t filler:14;
};

struct journal_taskexit
{
	uint32_t usec:20;           // 20 bits can hold 1 second duration
	uint32_t t_current:8;
	uint32_t jtype:4;
	
	uint32_t tid:8;             // 
	uint32_t tstate:4;
	uint32_t tisdelaying:1;
	uint32_t is_interrupt:1;
	uint32_t filler:18;
};

struct journal_mboxinit
{
	uint32_t usec:20;           // 20 bits can hold 1 second duration
	uint32_t t_current:8;
	uint32_t jtype:4;
	
	uint32_t usize:16;
	uint32_t unum:16;
};

struct journal_mboxdestroy
{
	uint32_t usec:20;           // 20 bits can hold 1 second duration
	uint32_t t_current:8;
	uint32_t jtype:4;

	uint32_t is_wakeup:1;
	uint32_t is_freedata:1;
	uint32_t filler:30;
};

struct journal_seminit
{
	uint32_t usec:20;           // 20 bits can hold 1 second duration
	uint32_t t_current:8;
	uint32_t jtype:4;
	
	uint32_t value;             // value of semaphore
};

struct journal_semdestroy
{
	uint32_t usec:20;           // 20 bits can hold 1 second duration
	uint32_t t_current:8;
	uint32_t jtype:4;

	uint32_t is_wakeup:1;
	uint32_t value:7;
	uint32_t filler:24;
};

struct journal_mtxinit
{
	uint32_t usec:20;           // 20 bits can hold 1 second duration
	uint32_t t_current:8;
	uint32_t jtype:4;
	
	uint32_t value:16;          // value of semaphore
	uint32_t filler:16;
};

struct journal_mtxdestroy
{
	uint32_t usec:20;           // 20 bits can hold 1 second duration
	uint32_t t_current:8;
	uint32_t jtype:4;

	uint32_t is_wakeup:1;
	uint32_t holdtimes:7;
	uint32_t value:24;
};

struct journal_msgqinit
{
	uint32_t usec:20;           // 20 bits can hold 1 second duration
	uint32_t t_current:8;
	uint32_t jtype:4;
	
	uint32_t size:16;             // size of messageq
	uint32_t filler:16;
};

struct journal_msgqdestroy
{
	uint32_t usec:20;           // 20 bits can hold 1 second duration
	uint32_t t_current:8;
	uint32_t jtype:4;

	uint32_t is_wakeup:1;
	uint32_t value:7;
	uint32_t filler:24;
};


typedef struct journal_class2
{
	uint32_t usec:20;           // 20 bits can hold 1 second duration in us
	uint32_t t_current:8;
	uint32_t jtype:4;

	uint32_t data;
}journal_class2_t;

#ifndef CRASHDUMP  // crash dump is a host tool

void journal_memory_alloc(uint32_t size, void *data, char *file, uint32_t line, task_t task);
void journal_memory_free(uint32_t size, void *data, char *file, uint32_t line, task_t task);
void journal_task_switch(struct dtask *from, struct dtask *to);
void journal_timestamp();
void journal_user_defined(uint32_t event_type, uint32_t data);
void journal_mbox_pend(mbox_t *mbox, int timeout);
void journal_mbox_post(mbox_t *mbox, dtask_t *task);
void journal_sem_pend(sem_t *sem, int timeout);
void journal_sem_post(sem_t *sem, dtask_t *task);

void journal_task_create(struct dtask *task);
void journal_task_exit(struct dtask *task);
void journal_mbox_init(mbox_t *mbox, uint16_t usize, uint16_t unum);
void journal_mbox_destroy(mbox_t *mbox);
void journal_sem_init(sem_t *sem);
void journal_sem_destroy(sem_t *sem);
void journal_mtx_init(mtx_t *mtx);
void journal_mtx_destroy(mtx_t *mtx);
void journal_msgq_init(msgq_t *msgq, uint16_t size);
void journal_msgq_destroy(msgq_t *msgq);


void journal_dump();

#endif

/**********************************************************************
 * Performance counters
 * 
 **********************************************************************/
#ifdef INCLUDE_PMCOUNTER
#define PMC_PEG_COUNTER(counter, value) \
	do{ \
		(counter) += (value); \
	}while(0)

#define PMC_SET_COUNTER(counter, value) \
		do{ \
			(counter) = (value); \
		}while(0)
#endif

/* system level counters */
enum
{
	PMC_U32_nTask,
	PMC_U32_nTaskReady,
	PMC_U32_nTaskSwitch,
	PMC_U32_nMbox,
	PMC_U32_nSemaphore,
	PMC_U32_nMutex,
	PMC_U32_nMessageQ,
	PMC_U32_nUprint,
	PMC_U32_nUprintOverwriten,
	PMC_U32_nInterruptInCurrentTick,// number ot interrupt in current ick
	PMC_U32_nInterruptInLastTick,	// number of interrupt in last tick
	PMC_U32_nsMaxJitterTick,		// max jitter in ns between one tick
	PMC_U32_nsAvgJitterTick,		// average jitter in ns between one tick
	PMC_U32_nsMaxInterruptResp,		// max interrupt response time in ns
	PMC_U32_nsAvgInterruptResp,		// average interrupt response time in ns

#if 0
	/* not available yet */
	PMC_U64_nDCacheMiss,
	PMC_U64_nICacheMiss,
	PMC_U32_nInstructionInLastTick,
#endif
	
	PMC_U32_nSysmax
};

/* memory counter */
enum{
	PMC_U32_nAlloc,
	PMC_U32_nFree,
	PMC_U32_nAllocFailed,
	PMC_U32_nFreeBytes,
	PMC_U32_nAllocRange1,
	PMC_U32_nFreeRange1,
	PMC_U32_nAllocFailedRange1,
	PMC_U32_nAllocRange2,
	PMC_U32_nFreeRange2,
	PMC_U32_nAllocFailedRange2,
	PMC_U32_nAllocRange3,
	PMC_U32_nFreeRange3,
	PMC_U32_nAllocFailedRange3,
	PMC_U32_nAllocRange4,
	PMC_U32_nFreeRange4,
	PMC_U32_nAllocFailedRange4,
	PMC_U32_nAllocRange5,
	PMC_U32_nFreeRange5,
	PMC_U32_nAllocFailedRange5,
	PMC_U32_nAllocRange6,
	PMC_U32_nFreeRange6,
	PMC_U32_nAllocFailedRange6,
	PMC_U32_nAllocRange7,
	PMC_U32_nFreeRange7,
	PMC_U32_nAllocFailedRange7,
	PMC_U32_nAllocRange8,
	PMC_U32_nFreeRange8,
	PMC_U32_nAllocFailedRange8,
};

/* task level counters */
enum{
	PMC_U32_Task_nsConsumedInLastSecond,
	PMC_U32_Task_nBytesOccupied,          // memory statistics
	PMC_U32_Task_nTaskSwitch,
	PMC_U32_Task_nMax
};

/* ipc level counters */


extern uint32_t PMC_sys32_counter[PMC_U32_nSysmax];

void pmc_calc_cpu_usage();
void pmc_dump();

#endif /* _JOURNAL_H_ */

