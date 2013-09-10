/*
 *  armboot - Startup Code for ARM720 CPU-core
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#define CONFIG_STACKSIZE 1024

/*
 *************************************************************************
 *
 * Jump vector table as in table 3.1 in [1]
 *
 *************************************************************************
 */


.globl _start
_start:	b       reset
	ldr	pc, _undefined_instruction
	ldr	pc, _software_interrupt
	ldr	pc, _prefetch_abort
	ldr	pc, _data_abort
	ldr	pc, _not_used
	ldr	pc, _irq
	ldr	pc, _fiq

_undefined_instruction:	.word undefined_instruction
_software_interrupt:	.word software_interrupt
_prefetch_abort:	.word prefetch_abort
_data_abort:		.word data_abort
_not_used:		.word not_used
_irq:			.word irq
_fiq:			.word fiq

	.balignl 16,0xdeadbeef


/*
 *************************************************************************
 *
 * Startup Code (reset vector)
 *
 * do important init only if we don't start from memory!
 * relocate armboot to ram
 * setup stack
 * jump to second stage
 *
 *************************************************************************
 */

/*
 * CFG_MEM_END is in the board dependent config-file (configs/config_BOARD.h)
 */
_TEXT_BASE:
	.word	TEXT_BASE

.globl _armboot_start
_armboot_start:
	.word _start

/*
 * Note: armboot_end is defined by the (board-dependent) linker script
 */
.globl _armboot_end
_armboot_end:
	.word armboot_end

/*
 * _armboot_real_end is the first usable RAM address behind armboot
 * and the various stacks
 */
.globl _armboot_real_end
_armboot_real_end:
	.word 0x0badc0de

/* IRQ stack memory (calculated at run-time) */
.globl IRQ_STACK_START
IRQ_STACK_START:
	.word _irq_stack_start+768

/* IRQ stack memory (calculated at run-time) */
.globl FIQ_STACK_START
FIQ_STACK_START:
	.word _abort_stack_start+64

.globl UNDEFINED_STACK_START
UNDEFINED_STACK_START:
	.word _abort_stack_start+64

.globl ABORT_STACK_START
ABORT_STACK_START:
	.word _abort_stack_start+64
	
/*
 * the actual reset code
 */

reset:
	/*
	 * set the cpu to SVC32 mode
	 */
	mrs	r0,cpsr
	bic	r0,r0,#0x1f
	orr	r0,r0,#0x13
	msr	cpsr,r0

	/*
	 * we do sys-critical inits only at reboot,
	 * not when booting from ram!
	 */
	/* bl	cpu_init_crit */
	bl     init_stacks

	ldr	pc, _start_dooloo

	/* never come back here */
	
relocate:
	/*
	 * relocate data section to RAM
	 */
	adr	r0, _start		/* r0 <- current position of code */
	ldr	r2, _armboot_start
	ldr	r3, _armboot_end
	sub	r2, r3, r2		/* r2 <- size of armboot */
	ldr	r1, _TEXT_BASE		/* r1 <- destination address */
	add	r2, r0, r2		/* r2 <- source end address */

	/*
	 * r0 = source address
	 * r1 = target address
	 * r2 = source end address
	 */
copy_loop:
	ldmia	r0!, {r3-r10}
	stmia	r1!, {r3-r10}
	cmp	r0, r2
	ble	copy_loop

	/* set up the stack */
	bl init_stacks

	#if 0
	ldr	r0, _armboot_end
	add	r0, r0, #CONFIG_STACKSIZE
	sub	sp, r0, #12		/* leave 3 words for abort-stack */
	#endif /* 0 */
	
	ldr	pc, _start_dooloo

_start_dooloo:	.word start_dooloo


/*
 *************************************************************************
 *
 * CPU_init_critical registers
 *
 * setup important registers
 * setup memory timing
 *
 *************************************************************************
 */

/* SYSCONs */
SYSCON1:	.word	0x80000100
SYSCON2:	.word	0x80001100
SYSCON3:	.word	0x80002200

#define CLKCTL	       0x6  /* mask */
#define CLKCTL_18      0x0  /* 18.432 MHz */
#define CLKCTL_36      0x2  /* 36.864 MHz */
#define CLKCTL_49      0x4  /* 49.152 MHz */
#define CLKCTL_73      0x6  /* 73.728 MHz */

cpu_init_crit:


#ifdef CONFIG_ARM7_REVD
	/* set clock speed */
	/* !!! we run @ 36 MHz due to a hardware flaw in Rev. D processors */
	/* !!! not doing DRAM refresh properly! */
	ldr	r0, SYSCON3
	ldr	r1, [r0]
	bic	r1, r1, #CLKCTL
	orr	r1, r1, #CLKCTL_36
	str	r1, [r0]
#endif

	mov	pc, lr




/*
 *************************************************************************
 *
 * Interrupt handling
 *
 *************************************************************************
 */

@
@ IRQ stack frame.
@
#define S_FRAME_SIZE	72

#define S_OLD_R0	68
#define S_PSR		64
#define S_PC		60
#define S_LR		56
#define S_SP		52

#define S_IP		48
#define S_FP		44
#define S_R10		40
#define S_R9		36
#define S_R8		32
#define S_R7		28
#define S_R6		24
#define S_R5		20
#define S_R4		16
#define S_R3		12
#define S_R2		8
#define S_R1		4
#define S_R0		0

#define MODE_SVC 0x13
#define I_BIT	 0x80

#define USERMODE	0x10
#define FIQMODE		0x11
#define IRQMODE		0x12
#define SVCMODE		0x13
#define ABORTMODE	0x17
#define UNDEFMODE	0x1b
#define MODEMASK	0x1f
#define NOINT		0xc0

/*
 * use bad_save_user_regs for abort/prefetch/undef/swi ...
 */
/* Porting notes: 
 * save the registers of before abort/prefetch/undef/swi ... into SVC stack
 * in descend order of: old r0, cpsr, pc, lr, sp, ip, fp, r10~r0
 * r0 = sp;
 */
	.macro	bad_save_user_regs
	sub	sp, sp, #S_FRAME_SIZE
	stmia	sp, {r0 - r12}			@ Calling r0-r12
	add     r8, sp, #S_PC

	ldr	r2, _armboot_end
	add	r2, r2, #CONFIG_STACKSIZE
	sub	r2, r2, #8
	ldmia	r2, {r2 - r4}                   @ get pc, cpsr, old_r0
	add	r0, sp, #S_FRAME_SIZE		@ restore sp_SVC

	add	r5, sp, #S_SP
	mov	r1, lr
	stmia	r5, {r0 - r4}                   @ save sp_SVC, lr_SVC, pc, cpsr, old_r
	mov	r0, sp
	.endm
/* Porting notes: 
 * 1) save the caller pc/cpsr (in lr/spsr) into _armboot_end + CONFIG_STACKSIZE
 * 2) switch to SVC mode
 */
	.macro get_bad_stack
	ldr	r13, _armboot_end		@ setup our mode stack
	add	r13, r13, #CONFIG_STACKSIZE	@ resides at top of normal stack
	sub	r13, r13, #8

	str	lr, [r13]			@ save caller lr / spsr
	mrs	lr, spsr
	str     lr, [r13, #4]

	mov	r13, #MODE_SVC			@ prepare SVC-Mode
	msr	spsr_c, r13
	mov	lr, pc
	movs	pc, lr
	.endm


/*
 * exception handlers
 */
	.align  5
undefined_instruction:
	get_bad_stack
	bad_save_user_regs
	bl 	do_undefined_instruction

	.align	5
software_interrupt:
	get_bad_stack
	bad_save_user_regs
	bl 	do_software_interrupt

	.align	5
prefetch_abort:
	get_bad_stack
	bad_save_user_regs
	bl 	do_prefetch_abort

	.align	5
data_abort:
	get_bad_stack
	bad_save_user_regs
	bl 	do_data_abort

	.align	5
not_used:
	get_bad_stack
	bad_save_user_regs
	bl 	do_not_used

	.align	5
.globl sys_interrupt_enter
.globl sys_interrupt_exit
.globl task_switch_interrput_flag
.globl interrupt_from_task
.globl interrupt_to_task
irq:
	stmfd sp!,{r0-r12,lr}
	bl sys_interrupt_enter
	bl do_irq
	bl sys_interrupt_exit

	/* if task_switch_interrput_flag set, jump to _interrupt_task_switch and don't return */
	ldr r0,=task_switch_interrput_flag
	ldr r1,[r0]
	cmp r1,#1
	beq _interrupt_task_switch

	ldmfd sp!,{r0-r12,lr}
    subs pc,lr,#4

	.align	5
fiq:
	stmfd sp!,{r0-r7,lr}
	bl 	do_fiq
	ldmfd sp!,{r0-r7,lr}
    subs pc,lr,#4

_interrupt_task_switch:
	mov r1,#0					@ clear task_switch_interrput_flag
	str r1,[r0]

	ldmfd sp!,{r0-r12,lr}		@ reload saved registers
	stmfd sp!,{r0-r3}			@ save r0-r3
	mov r1,sp
	add sp,sp,#16				@ restore sp
	sub r2,lr,#4				@ save old task's pc to r2

	mrs r3,spsr					@ disable interrupt
	orr r0,r3,#NOINT
	msr spsr_c,r0

	ldr r0,=.+8					@ switch to interrupted task's stack
	movs pc,r0

	stmfd sp!,{r2}              @ push old task's pc
	stmfd sp!,{r4-r12,lr}       @ push old task's lr,r12-r4
	mov r4,r1                   @ Special optimised code below
	mov r5,r3
	ldmfd r4!,{r0-r3}
	stmfd sp!,{r0-r3}           @ push old task's r3-r0
	stmfd sp!,{r5}              @ push old task's psr
	mrs r4,spsr
	stmfd sp!,{r4}              @ push old task's spsr
	
	ldr r4,=interrupt_from_task
	ldr r5,[r4]
	str sp,[r5]                 @ store sp in preempted tasks's TCB

/*	bl OSTaskSwHook             @ call Task Switch Hook */

	ldr r6,=interrupt_to_task
	ldr r6,[r6]
	ldr sp,[r6]                 @ get new task's stack pointer

	ldmfd sp!,{r4}              @ pop new task's spsr
	msr SPSR_cxsf,r4
	ldmfd sp!,{r4}              @ pop new task's psr
	msr CPSR_cxsf,r4

	ldmfd sp!,{r0-r12,lr,pc}    @ pop new task's r0-r12,lr & pc

init_stacks:
	mrs r0, cpsr
	bic r0, r0, #MODEMASK
	orr r1, r0, #UNDEFMODE|NOINT
	msr cpsr_cxsf, r1						/* undef mode */
	ldr sp, UNDEFINED_STACK_START
	
	orr r1,r0,#ABORTMODE|NOINT
	msr cpsr_cxsf,r1						/* abort mode */
	ldr sp, ABORT_STACK_START

	orr r1,r0,#IRQMODE|NOINT
	msr cpsr_cxsf,r1            			/* IRQ mode */
	ldr sp, IRQ_STACK_START

	orr r1,r0,#FIQMODE|NOINT
	msr cpsr_cxsf,r1            			/* FIQ mode */
	ldr sp, FIQ_STACK_START

	bic r0,r0,#MODEMASK|NOINT
	orr r1,r0,#SVCMODE|NOINT
	msr cpsr_cxsf,r1            			/* SVC mode */
	ldr	r0, _armboot_end
	add	sp, r0, #CONFIG_STACKSIZE

	@ USER mode is not initialized.

	mov pc,lr        @ The LR register may be not valid for the mode changes.
	
					
	.align	5
.globl reset_cpu
reset_cpu:
	mov     pc, r0

