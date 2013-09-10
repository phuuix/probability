/*
 * os_support_c.c
 * OS support C functions
 */
#include <bsp.h>

/* the two variables are used in bsp_task_switch_interrupt() function */
uint32_t interrupt_from_task, interrupt_to_task;
uint32_t task_switch_interrput_flag;

char *bsp_task_init(int t, void (*task)(void *p), void *parm,
	char *stack, void (*killer)(void))
{
	uint32_t *stk;

	stk = (uint32_t *)stack;
	*(stk) = (uint32_t)task;			/* entry point */
	*(--stk) = (uint32_t)killer;		/* lr */
	*(--stk) = 0;						/* r12 */
	*(--stk) = 0;						/* r11 */
	*(--stk) = 0;						/* r10 */
	*(--stk) = 0;						/* r9 */
	*(--stk) = 0;						/* r8 */
	*(--stk) = 0;						/* r7 */
	*(--stk) = 0;						/* r6 */
	*(--stk) = 0;						/* r5 */
	*(--stk) = 0;						/* r4 */
	*(--stk) = 0;						/* r3 */
	*(--stk) = 0;						/* r2 */
	*(--stk) = 0;						/* r1 */
	*(--stk) = uint32_t)parm;			/* r0 : argument */
	*(--stk) = SVCMODE;					/* cpsr */
	*(--stk) = SVCMODE;					/* spsr */

	/* return task's current stack address */
	return (char *)stk;
}

