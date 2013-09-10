/*
 * isr.c
 * Interrupt service routines
 */

#include <isr.h>
#include <stdio.h>
#include <bsp.h>
#include <i386_hw.h>
#include <task.h>

#define MAX_HANDLERS	16

/* exception and interrupt handler table */
unsigned long esr_table[MAX_HANDLERS];
unsigned long isr_table[MAX_HANDLERS];

/* dummy exception service function */
static void dummy_esr_fun(int i)
{
	extern int gdb_registers[];
	extern struct dtask *current;
	unsigned long err, eip, eflags, eax, ecx, edx, ebx, esp, ebp, esi, edi;
	unsigned long *psp;

	err = 0;
	psp = (unsigned long *)(gdb_registers);

	if((i==8) || (i==10) || (i==11) || (i==12) || (i==13) || (i==14))
	{
		/* error code */
		err = *(psp+15);
	}

	eax = *(psp++);
	ecx = *(psp++);
	edx = *(psp++);
	ebx = *(psp++);
	esp = *(psp++);
	ebp = *(psp++);
	esi = *(psp++);
	edi = *(psp++);

	eip = *(psp++);
	eflags = *(psp++);
	
	kprintf("Unhandled exception %d occured!!!\n", i);
	kprintf("current task is %s\n", current->name);
	kprintf("eip(%x) eflags(%x) error code(%x)\n", eip, eflags, err);
	kprintf("eax(%x) ebx(%x) ecx(%x) edx(%x)\n", eax, ebx, ecx, edx);
	kprintf("esp(%x) ebp(%x) esi(%x) edi(%x)\n", esp, ebp, esi, edi);
	while(1);
}


/* dummy interrupt service function */
static void dummy_isr_fun(int i)
{
	kprintf("Unhandled interrupt %d occured!!!\n", i);
	while(1);
}

/* init exception handler table */
void esr_init()
{
	int i;
	
	/* init exceptions table */
	for(i=0; i<MAX_HANDLERS; i++)
	{
		esr_table[i] = (uint32_t)dummy_esr_fun;
	}
}


void bsp_esr_attach(uint16_t index, void (*handler)(int n))
{
	if(index < MAX_HANDLERS)
		esr_table[index] = (uint32_t)handler;
}


void bsp_esr_detach(uint16_t index)
{
	if(index < MAX_HANDLERS)
		esr_table[index] = (uint32_t)dummy_esr_fun;
}


/* init exception handler table */
void isr_init()
{
	int i;
	
	/* init exceptions table */
	for(i=0; i<MAX_HANDLERS; i++)
	{
		isr_table[i] = (uint32_t)dummy_isr_fun;
	}
}


/* attach isr */
void bsp_isr_attach(uint16_t index, void (*handler)(int n))
{
	if(index < MAX_HANDLERS)
		isr_table[index] = (uint32_t)handler;
}


/* deattach isr */
void bsp_isr_detach(uint16_t index)
{
	if(index < MAX_HANDLERS)
		isr_table[index] = (uint32_t)dummy_isr_fun;
}
