/*
 * i386_tasksw.h
 */

#ifndef __D_I386_TASKSW_H__
#define __D_I386_TASKSW_H__

#include <sys/types.h>

/* max task number */
#define TSSMAX 100
/* tss base */
#define TSS_BASE 100

#define TASK2SELECTOR(t) 8*(TSS_BASE +i)

void init_TR(WORD c);
void context_load(WORD c);
WORD context_save();

WORD i386_switch_from(void);

#endif /* __D_I386_TASKSW_H__ */
