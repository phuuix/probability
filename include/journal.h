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
#define JOURNAL_CLASS1_MAXRECORD  512
#define JOURNAL_CLASS2_MAXRECORD  16

enum journal_type
{
	JOURNAL_TYPE_TASKSWITCH,
	JOURNAL_TYPE_IPCPOST,
	JOURNAL_TYPE_IPCPEND,
	JOURNAL_TYPE_TASKCREATE,
	JOURNAL_TYPE_TASKEXIT,
	JOURNAL_TYPE_IPCINIT,
	JOURNAL_TYPE_IPCDESTROY,
    JOURNAL_TYPE_TIMESTAMP,
};

struct journal_taskswitch
{
	uint32_t time;
	uint8_t jtype;
	uint8_t tfrom_state;
	uint8_t tfrom_flags;
	uint8_t tto_wakeup_cause;
	uint16_t t_from;
	uint16_t t_to;
};

struct journal_ipcop
{
	uint32_t time;
	uint8_t jtype;
	uint8_t ipctype;
    uint8_t active_ints;
	uint8_t tid;
	ipc_t *ipc_ptr;
};

struct journal_time
{
    uint32_t time;    // ns
    uint8_t jtype;
    uint8_t filler;  
    uint16_t cur_tid; // current task
    uint32_t tv_sec;  // second
};

typedef union journal_class1
{
	struct journal_taskswitch taskswitch;
	struct journal_ipcop ipcop;
    struct journal_time timestamp;
}journal_class1_t;

struct journal_tasklife
{
	uint32_t time;
	uint8_t jtype;
	uint8_t flags;
	uint16_t filler;
	uint16_t tid;
	uint16_t t_parent;
};

struct journal_ipclife
{
	uint32_t time;
	uint8_t jtype;
	uint8_t ipctype;
	uint16_t tid;
	ipc_t *ipc_ptr;
};

typedef union journal_class2
{
	struct journal_tasklife tasklife;
	struct journal_ipclife ipclife;
}journal_class2_t;


void journal_memory_alloc(uint32_t size, void *data, char *file, uint32_t line, task_t task);
void journal_memory_free(uint32_t size, void *data, char *file, uint32_t line, task_t task);
void journal_task_switch(struct dtask *from, struct dtask *to);
void journal_ipc_pend(ipc_t *ipc, task_t task);
void journal_ipc_post(ipc_t *ipc, task_t task);
void journal_timestamp();
void journal_user_defined(uint32_t data1, uint32_t data2);


void journal_task_create(struct dtask *task);
void journal_task_exit(struct dtask *task);
void journal_ipc_init(ipc_t *ipc);
void journal_ipc_destroy(ipc_t *ipc);
void journal_dump();

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

