/*
 * (C) Copyright 2002
 * Sysgo Real-Time Solutions, GmbH <www.elinos.com>
 * Marius Groeger <mgroeger@sysgo.de>
 *
 * (C) Copyright 2002
 * Sysgo Real-Time Solutions, GmbH <www.elinos.com>
 * Alex Zuepke <azu@sysgo.de>
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

#include <bsp.h>
#include <stddef.h>
#include "task.h"
#include "lpc2xxx.h"
#include "serial.h"
#include "interrupts.h"
#include "memory.h"

#define MAX_HANDLERS	16
#define HANDLE_MASK 0xF

/* exception and interrupt handler table */
static uint32_t isr_table[MAX_HANDLERS];
//static void (*isr_table)()[MAX_HANDLERS];
//static unsigned long interrupt_mask;

/* we always count down the max. */
#define TIMER_LOAD_VAL 0xffff

extern struct dtask *current;


/* dummy interrupt service function */
static void dummy_isr_fun(int i)
{
	kprintf("Unhandled interrupt %d occured!!!\n", i);
}

/*******************************************************************************
 * BSP interface to lpc221x
 ******************************************************************************/
void bsp_irq_mask(uint32_t irqno)
{
	irqno &= HANDLE_MASK;
	VICIntEnClr = 1 << irqno;
}

void bsp_irq_unmask(uint32_t irqno)
{
	irqno &= HANDLE_MASK;
	VICIntEnable = (1 << irqno);
}

/* attach isr */
void bsp_isr_attach(uint16_t index, void (*handler)(int n))
{
	if(index < MAX_HANDLERS)
	{
		*(volatile uint32_t *)(VICVectAddrBase+index*4) = (uint32_t)handler;
		*(volatile uint32_t *)(VICVectCntlBase+index*4) = (uint32_t)(LPCBIT5|index);
  
		isr_table[index] = (uint32_t)handler;
	}
}


/* deattach isr */
void bsp_isr_detach(uint16_t index)
{
	if(index < MAX_HANDLERS)
		isr_table[index] = (uint32_t)dummy_isr_fun;
}


void bsp_esr_attach(uint16_t index, void (*handler)(int n))
{
}

void bsp_esr_detach(uint16_t index)
{
}

/* enable IRQ/FIQ interrupts */
void bsp_int_enable(void)
{
    unsigned long temp;
    __asm__ __volatile__("mrs %0, cpsr\n"
			 "bic %0, %0, #0x80\n"
			 "msr cpsr_c, %0"
			 : "=r" (temp)
			 :
			 : "memory");
}


/* 
 * disable IRQ/FIQ interrupts
 * returns true if interrupts had been enabled before we disabled them
 */
int bsp_int_disable(void)
{
    unsigned long old,temp;
    __asm__ __volatile__("mrs %0, cpsr\n"
			 "orr %1, %0, #0x80\n"
			 "msr cpsr_c, %1"
			 : "=r" (old), "=r" (temp)
			 :
			 : "memory");
    return (old & 0x80) == 0;
}


void dump_reg_frame(unsigned long *reg_frame)
{
	kprintf("r0~r4: 0x%x 0x%x 0x%x 0x%x 0x%x\n",
		reg_frame[0], reg_frame[1], reg_frame[2], reg_frame[3], reg_frame[4]);
	kprintf("r5~r10: 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\n",
		reg_frame[5], reg_frame[6], reg_frame[7], reg_frame[8], reg_frame[9], reg_frame[10]);
	kprintf("fp=0x%x ip=0x%x sp=0x%x lr=0x%x pc=0x%x cpsr=0x%x\n",
		reg_frame[11], reg_frame[12], reg_frame[13],
		reg_frame[14], reg_frame[15], reg_frame[16]);
}

/*
 * fp: top of innerest stack
 * low: low value of stack
 * high: high vlaue of stack
 */
void dump_call_stack(unsigned long fp, unsigned long low, unsigned long high)
{
	unsigned long *pStack, lr, loops = 0;

	kprintf("calling stacks:\n");
	while(fp>=low && fp<=high && loops <20)
	{
		/* {fp, ip, lr, pc} */
		pStack = (unsigned long *)(fp-12);
		//pc = pStack[3];
		lr = pStack[2];
		//ip = pStack[1];
		fp = pStack[0];
		kprintf("%d -- 0x%x \n", loops, lr);
		loops ++;
	}
	
}

void panic(char *infostr)
{
	bsp_fsave();
	
	kprintf(infostr);
	
	if(current)
	{
		/* dump current task's state */
		kprintf("current=0x%x last sp=0x%x stack_base=0x%x taskname=%s state=%d\n", 
			current, current->sp, current->stack_base, current->name, current->state);
	}

	/* dump system ready queue */

	/* dump memory usage */
	memory_dump();
	
    while(1);
}

void do_undefined_instruction(unsigned long *reg_frame)
{
	dump_reg_frame(reg_frame);
	// fp save the top of innerest stack
	dump_call_stack(reg_frame[11], MEMLOW, MEMHIGH);
    panic("undefined instruction\n");
}

void do_software_interrupt(unsigned long *reg_frame)
{
	dump_reg_frame(reg_frame);
	// fp save the top of innerest stack
	dump_call_stack(reg_frame[11], MEMLOW, MEMHIGH);
    panic("software interrupt\n");
}

void do_prefetch_abort(unsigned long *reg_frame)
{
	dump_reg_frame(reg_frame);
	// fp save the top of innerest stack
	dump_call_stack(reg_frame[11], MEMLOW, MEMHIGH);
    panic("prefetch abort\n");
}

void do_data_abort(unsigned long *reg_frame)
{
	dump_reg_frame(reg_frame);
	// fp save the top of innerest stack
	dump_call_stack(reg_frame[11], MEMLOW, MEMHIGH);
    panic("data abort\n");
}

void do_not_used(unsigned long *reg_frame)
{
	dump_reg_frame(reg_frame);
	// fp save the top of innerest stack
	dump_call_stack(reg_frame[11], MEMLOW, MEMHIGH);
    panic("not used\n");
}

void do_fiq(unsigned long *reg_frame)
{
	dump_reg_frame(reg_frame);
	// fp save the top of innerest stack
	dump_call_stack(reg_frame[11], MEMLOW, MEMHIGH);
    panic("fast interrupt request\n");
}

typedef void (*ISR_FUNC)();
void do_irq(void)
{
	unsigned long intstat, lsr;
	ISR_FUNC isr_func;
	//void (*isr_func)() = NULL;

	intstat = VICIRQStatus;
	isr_func = (ISR_FUNC)VICVectAddr;
	if(intstat & (1<<INTSRC_UART0))
	{
		//kprintf("UART0 interrupt\n");
		/* get isr_func for sure */
		isr_func = (void (*)()) isr_table[INTSRC_UART0];
		
		/* line status */
		lsr = U0LSR;
		if(lsr & (LPCBIT1|LPCBIT2|LPCBIT3|LPCBIT7))
			lsr = U0RBR;	// read RBR to clear interrupt
		else if(lsr & LPCBIT0)
			isr_func(INTSRC_UART0);
		else
			lsr = U0RBR;	// read RBR to clear interrupt
	}
	else if(intstat & (1<<INTSRC_UART1))
	{
		//kprintf("UART1 interrupt\n");
		isr_func = (void (*)()) isr_table[INTSRC_UART1];
		/* line status */
		lsr = U1LSR;
		if(lsr & (LPCBIT1|LPCBIT2|LPCBIT3|LPCBIT7))
			lsr = U1RBR;	// read RBR to clear interrupt
		else if(lsr & LPCBIT0)
			isr_func(INTSRC_UART1);
		else
			lsr = U1RBR;	// read RBR to clear interrupt
	}
	else if(intstat & (1<<INTSRC_TIMER0))
	{
		/* clear all interrupts */
		T0IR = 0xFF;
		isr_func = (void (*)()) isr_table[INTSRC_TIMER0];
		isr_func(INTSRC_TIMER0);
	}
	else if(intstat & (1<<INTSRC_TIMER1))
	{
		kprintf("TIMER1 interrupt\n");
	}
	else if(intstat & (1<<INTSRC_EINT0))
	{
		isr_func = (void (*)()) isr_table[INTSRC_EINT0];
		isr_func(INTSRC_EINT0);
		/* clear interrupt */
		EXTINT = 0x01;
	}
	else
	{
    	kprintf("interrupt request state: 0x%x\n", intstat);
	}

	/* acknowledge interrupt */
	VICVectAddr = 0;
	//VICDefVectAddr = 0;
}

