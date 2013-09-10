/*
 * i386_tasksw.c
 * Deal with i386 task switch
 */
#include <sys/types.h>
#include <task.h>

#include <i386_data.h>
#include <i386_sel.h>
#include <i386_tasksw.h>
#include <i386_hw.h>


TSS	TSS_table[TSSMAX];


/* Initialize context -> TSS in 32 bit */
char *bsp_task_init(WORD index, void (*task)(void *p), void *parm, BYTE *stack,void (*killer)(void))
{
    DWORD *stack_ptr = (DWORD *)stack;

    /* Push onto stack the input parameter              */
    /* And the entry point to the task killer procedure */
    stack_ptr--;
    *stack_ptr = (DWORD)(parm);
    stack_ptr--;
    *stack_ptr = (DWORD)(killer);

    /* Fill the TSS structure */
    /* No explicit protection; use only one stack */
    TSS_table[index].esp0 = (DWORD)(stack_ptr);
    TSS_table[index].esp1 = 0;
    TSS_table[index].esp2 = 0;
    TSS_table[index].ss0 = X_FLATDATA_SEL;
    TSS_table[index].ss1 = 0;
    TSS_table[index].ss2 = 0;
    /* No paging activated */
    TSS_table[index].cr3 = 0;
    /* Task entry point */
    TSS_table[index].eip = (DWORD)(task);
    /* Need only to unmask interrupts */
    TSS_table[index].eflags = 0x00000200;
    //TSS_table[index].eflags = 0x00000000;
    TSS_table[index].eax = 0;
    TSS_table[index].ebx = 0;
    TSS_table[index].ecx = 0;
    TSS_table[index].edx = 0;
    TSS_table[index].esi = 0;
    TSS_table[index].edi = 0;
    TSS_table[index].esp = (DWORD)(stack_ptr);
    TSS_table[index].ebp = (DWORD)(stack_ptr);
    TSS_table[index].cs = X_FLATCODE_SEL;
    TSS_table[index].es = X_FLATDATA_SEL;
    TSS_table[index].ds = X_FLATDATA_SEL;
    TSS_table[index].ss = X_FLATDATA_SEL;
    TSS_table[index].gs = X_FLATDATA_SEL;
    TSS_table[index].fs = X_FLATDATA_SEL;
    /* Use only the GDT */
    TSS_table[index].ldt = 0;
    TSS_table[index].trap = 0;
    TSS_table[index].io_base = 0;

	/* return task's current stack address, but this address isn't used under i386 arch */
	return (char *)stack_ptr;
}

void bsp_task_switch(unsigned long from, unsigned long to)
{
	struct {long a,b;} __tmp;
	__tmp.a = 0;
	__tmp.b = (TSS_BASE + TASK_T((struct dtask *)to)) << 3;
	//kprintf("__tmp = {%d %d}\n", __tmp.a, __tmp.b);
	asm ("ljmp *%0": :"m" (*&__tmp));
}

void bsp_task_switch_interrupt(unsigned long from, unsigned long to)
{
	bsp_task_switch(from, to);
}

/*
WORD i386_switch_from(void)
{
	return get_TR();
}
*/
