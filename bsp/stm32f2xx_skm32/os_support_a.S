/*
 * os_support.S
 * 
 * Contains os support functions.
 */
  .syntax unified
  .cpu cortex-m3
  .fpu softvfp
  .thumb

NVIC_INT_CTRL:  .word 0xE000ED04                                   @ Interrupt control state register.
NVIC_PENDSVSET: .word 0x10000000                                   @ Value to trigger PendSV exception.

/*
 * unsigned int bsp_fsave();
 */
.globl bsp_fsave
bsp_fsave:
	mrs r0, primask
	cpsid i
	bx lr

/*
 * void bsp_frestore(unsigned int flags);
 */
.globl bsp_frestore
bsp_frestore:
	msr     primask, r0
    	bx      lr


/*
 * void bsp_task_switch(uint32_t from, uint32_t to);
 * r0 --> from
 * r1 --> to
 */
.globl bsp_task_switch
bsp_task_switch:
	ldr r2,=interrupt_from_task	@ set interrupt_from_task
	str r0, [r2]
	ldr r2,=interrupt_to_task	@ set interrupt_to_task
	str r1, [r2]
	ldr     r0, NVIC_INT_CTRL                                  @ Trigger the PendSV exception (causes context switch)
	ldr     r1, NVIC_PENDSVSET
	str     r1, [r0]
	bx      lr
	

/*
 * void bsp_task_switch_interrupt(uint32_t from, uint32_t to);
 */
.globl interrupt_from_task
.globl interrupt_to_task
.globl bsp_task_switch_interrupt
bsp_task_switch_interrupt:
	ldr r2,=interrupt_from_task	@ set interrupt_from_task
	str r0, [r2]
	ldr r2,=interrupt_to_task	@ set interrupt_to_task
	str r1, [r2]
	ldr     r0, NVIC_INT_CTRL                                  @ Trigger the PendSV exception (causes context switch)
	ldr     r1, NVIC_PENDSVSET
	str     r1, [r0]
	bx      lr


