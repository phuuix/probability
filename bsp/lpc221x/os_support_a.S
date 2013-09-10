/*
 * os_support.S
 * 
 * Contains os support functions.
 */

#define NOINT		0xc0

/*
 * unsigned int bsp_fsave();
 */
.globl bsp_fsave
bsp_fsave:
	mrs r0, cpsr
	orr r1, r0, #NOINT
	msr cpsr_c, r1
	mov pc, lr

/*
 * void bsp_frestore(unsigned int flags);
 */
.globl bsp_frestore
bsp_frestore:
	msr cpsr, r0
	mov pc, lr


/*
 * void bsp_task_switch(uint32_t from, uint32_t to);
 * r0 --> from
 * r1 --> to
 */
.globl bsp_task_switch
bsp_task_switch:
	stmfd sp!, {lr}				@ push pc (lr should be pushed in place of PC)
	stmfd sp!, {r0-r12, lr}		@ push lr & register file
	mrs r4, cpsr
	stmfd sp!, {r4}				@ push cpsr
	mrs r4, spsr
	stmfd sp!, {r4}				@ push spsr

	str sp, [r0]				/* store sp in preempted tasks's TCB */
	ldr	sp, [r1]				/* get new task's stack pointer */

	ldmfd sp!, {r4}				/* pop new task's spsr */
	msr spsr_cxsf, r4
	ldmfd sp!, {r4}				/* pop new task's cpsr */
	msr cpsr_cxsf, r4
	ldmfd sp!, {r0-r12, lr, pc}	/* pop new task's r0-r12, lr & pc */
	

/*
 * void bsp_task_switch_interrupt(uint32_t from, uint32_t to);
 */
.globl task_switch_interrput_flag
.globl interrupt_from_task
.globl interrupt_to_task
.globl bsp_task_switch_interrupt
bsp_task_switch_interrupt:
	ldr r2, =task_switch_interrput_flag
	ldr r3,[r2]
	cmp r3,#1
	beq reswitch_interrupt
	mov r3, #1					@ set task_switch_interrupt_flag to 1
	str r3, [r2]
	ldr r2,=interrupt_from_task	@ set interrupt_from_task
	str r0, [r2]
reswitch_interrupt:
	ldr r2,=interrupt_to_task	@ set interrupt_to_task
	str r1, [r2]
	mov pc, lr
