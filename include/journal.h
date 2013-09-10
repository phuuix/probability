/* journal.h 
 * Journal and PMC (performance counter)
 */
#ifndef _JOURNAL_H_
#define _JOURNAL_H_

#include "ipc.h"

/**********************************************************************
 * Journal
 * 
 **********************************************************************/

enum journal_type
{
	JOURNAL_TYPE_TASKSWITCH,
	JOURNAL_TYPE_IPCPOST,
	JOURNAL_TYPE_IPCPEND,
	JOURNAL_TYPE_TASKCREATE,
	JOURNAL_TYPE_TASKEXIT,
	JOURNAL_TYPE_IPCINIT,
	JOURNAL_TYPE_IPCDESTROY,
};

struct journal_taskswitch
{
	uint32_t time;
	uint8_t jtype;
	uint16_t t_from;
	uint16_t t_to;
};

struct journal_ipcop
{
	uint32_t time;
	uint8_t jtype;
	uint8_t ipctype;
	uint16_t tid;
	ipc_t *ipc_ptr;
};

typedef union journal_class1
{
	struct journal_taskswitch taskswitch;
	struct journal_ipcop ipcop;
}journal_class1_t;

struct journal_tasklife
{
	uint32_t time;
	uint8_t jtype;
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

void journal_task_create(struct dtask *task);
void journal_task_exit(struct dtask *task);
void journal_ipc_init(ipc_t *ipc);
void journal_ipc_destroy(ipc_t *ipc);

/**********************************************************************
 * Performance counters
 * 
 **********************************************************************/
#ifdef PMCounter
#define PMC_PEG_SYS32_COUNTER(counter, value) \
	do{ \
		PMC_sys32_counter[counter] += value; \
	}while(0)
#else
#define PMC_PEG_SYS32_COUNTER(counter, value)
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
	PMC_U32_Task_nBytesOccupied,
	PMC_U32_Task_nTaskSwitch,
};

/* ipc level counters */


extern uint32_t PMC_sys32_counter[PMC_U32_nSysmax];

#endif /* _JOURNAL_H_ */

