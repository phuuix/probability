
/* system journal  and performance counter */
#include <string.h>

#include "bsp.h"
#include "memory.h"
#include "task.h"
#include "clock.h"
#include "journal.h"
#include "uprintf.h"

#define JOURNAL_CLASS1_MAXRECORD  32
#define JOURNAL_CLASS1_NEXTRECORD (&journal_class1_history[journal_class1_index]);
#define JOURNAL_CLASS1_UPDATENEXT() \
	do{ \
		journal_class1_index = ((journal_class1_index+1)>=JOURNAL_CLASS1_MAXRECORD)?0:(journal_class1_index+1); \
	}while(0)

#define JOURNAL_CLASS2_MAXRECORD  32
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
	uprintf(UPRINT_INFO, BLOCK_MEM, "Alloc: size=%d data=0x%x task=%d\n", size, data, task);

	// TODO:  save data block's attribute
}

void journal_memory_free(uint32_t size, void *data, char *file, uint32_t line, task_t task)
{
	uprintf(UPRINT_INFO, BLOCK_MEM, "Free: size=%d data=0x%x task=%d\n", size, data, task);

	// TODO:  save data block's attribute
}

/**************************************************************************
 * CLASS1 journal
 **************************************************************************/
void journal_task_switch(struct dtask *from, struct dtask *to)
{
	uprintf(UPRINT_INFO, BLOCK_TASK, "Task switch: from(%s: sp=0x%x) to(%s: sp=0x%x)\n", 
			from->name, from->sp, to->name, to->sp);

	// save task switch history
	struct journal_taskswitch *jtaskswitch = (struct journal_taskswitch *)JOURNAL_CLASS1_NEXTRECORD;
	jtaskswitch->time = tick();
	jtaskswitch->jtype = JOURNAL_TYPE_TASKSWITCH;
	jtaskswitch->t_from = TASK_T(from);
	jtaskswitch->t_to = TASK_T(to);

	JOURNAL_CLASS1_UPDATENEXT();
}

void journal_ipc_pend(ipc_t *ipc, task_t task)
{
	uprintf(UPRINT_INFO, BLOCK_IPC, "IPC pend: type=%d ipc=0x%x task=%d\n", ipc->type, ipc, task);

	// save ipc history
	struct journal_ipcop *jipcop = (struct journal_ipcop *)JOURNAL_CLASS1_NEXTRECORD;
	jipcop->time = tick();
	jipcop->ipc_ptr = ipc;
	jipcop->ipctype = ipc->type;
	jipcop->tid = TASK_T(current);
	jipcop->jtype = JOURNAL_TYPE_IPCPEND;

	JOURNAL_CLASS1_UPDATENEXT();
}


void journal_ipc_post(ipc_t *ipc, task_t task)
{
	uprintf(UPRINT_INFO, BLOCK_IPC, "IPC post: type=%d ipc=0x%x task=%d\n", ipc->type, ipc, task);

	// save ipc history
	struct journal_ipcop *jipcop = (struct journal_ipcop *)JOURNAL_CLASS1_NEXTRECORD;
	jipcop->time = tick();
	jipcop->ipc_ptr = ipc;
	jipcop->ipctype = ipc->type;
	jipcop->tid = TASK_T(current);
	jipcop->jtype = JOURNAL_TYPE_IPCPOST;

	JOURNAL_CLASS1_UPDATENEXT();
}

/**************************************************************************
 * CLASS2 journal
 **************************************************************************/
void journal_task_create(struct dtask *task)
{
	uprintf(UPRINT_INFO, BLOCK_TASK, "Task create: %s: sp=0x%x\n", task->name, task->sp);

	struct journal_tasklife *jtasklife = (struct journal_tasklife *)JOURNAL_CLASS2_NEXTRECORD;
	jtasklife->time = tick();
	jtasklife->tid = TASK_T(task);
	jtasklife->jtype = JOURNAL_TYPE_TASKCREATE;
	jtasklife->t_parent = TASK_T(current);

	JOURNAL_CLASS2_UPDATENEXT();
}


void journal_task_exit(struct dtask *task)
{
	uprintf(UPRINT_INFO, BLOCK_TASK, "Task exit: %s: sp=0x%x\n", task->name, task->sp);

	struct journal_tasklife *jtasklife = (struct journal_tasklife *)JOURNAL_CLASS2_NEXTRECORD;
	jtasklife->time = tick();
	jtasklife->tid = TASK_T(task);
	jtasklife->jtype = JOURNAL_TYPE_TASKEXIT;
	jtasklife->t_parent = TASK_T(current);

	JOURNAL_CLASS2_UPDATENEXT();
}

void journal_ipc_init(ipc_t *ipc)
{
	uprintf(UPRINT_INFO, BLOCK_IPC, "IPC init: %s: type=%d: ptr=0x%x\n", ipc->type, ipc);

	struct journal_ipclife *jipclife = (struct journal_ipclife *)JOURNAL_CLASS2_NEXTRECORD;
	jipclife->time = tick();
	jipclife->tid = TASK_T(current);
	jipclife->jtype = JOURNAL_TYPE_IPCINIT;
	jipclife->ipctype = ipc->type;
	jipclife->ipc_ptr = ipc;

	JOURNAL_CLASS2_UPDATENEXT();
}

void journal_ipc_destroy(ipc_t *ipc)
{
	uprintf(UPRINT_INFO, BLOCK_IPC, "IPC destroy: type=%d: ptr=0x%x\n", ipc->type, ipc);

	struct journal_ipclife *jipclife = (struct journal_ipclife *)JOURNAL_CLASS2_NEXTRECORD;
	jipclife->time = tick();
	jipclife->tid = TASK_T(current);
	jipclife->jtype = JOURNAL_TYPE_IPCDESTROY;
	jipclife->ipctype = ipc->type;
	jipclife->ipc_ptr = ipc;

	JOURNAL_CLASS2_UPDATENEXT();
}

/**************************************************************************
 * System level counterl
 **************************************************************************/
uint32_t PMC_sys32_counter[PMC_U32_nSysmax];
