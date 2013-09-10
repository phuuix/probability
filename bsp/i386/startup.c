/*
 * startup.c
 *   The first c file called by os
 */
#include <config.h>
#include <version.h>
#include <multiboot.h>
#include <stdio.h>
#include <irq.h>
#include <bsp.h>
#include <isr.h>
#include <pit.h>
#include <task.h>
#include <lock.h>
#include <list.h>
#include <ipc.h>
#include <assert.h>
#include <clock.h>
#include <idletask.h>
#include <shell.h>
#include <s_alloc.h>
#include <command.h>
#include <memory.h>
#include <3c509.h>
#include <i386_stub.h>
#include <pci.h>
#include <rtl8139.h>
#include <tty.h>

#include <net/net_task.h>

#include "i386_data.h"
#include "i386_tasksw.h"
#include "i386_cons.h"
#include "i386_keyboard.h"

extern void exc0(void);
extern void exc1(void);
extern void exc2(void);
extern void exc3(void);
extern void exc4(void);
extern void exc5(void);
extern void exc6(void);
extern void exc7(void);
extern void exc8(void);
extern void exc9(void);
extern void exc10(void);
extern void exc11(void);
extern void exc12(void);
extern void exc13(void);
extern void exc14(void);
extern void exc15(void);
extern void exc16(void);

/* hardware interrupt hooks  */
extern void h0(void);
extern void h1(void);
extern void h2(void);
extern void h3(void);
extern void h4(void);
extern void h5(void);
extern void h6(void);
extern void h7(void);
extern void h8(void);
extern void h9(void);
extern void h10(void);
extern void h11(void);
extern void h12(void);
extern void h13(void);
extern void h13_bis(void);
extern void h14(void);
extern void h15(void);

extern TSS TSS_table[];

#include <linkage.h>
#include <string.h>
extern void *SYMBOL_NAME(__bss_start);
extern void *SYMBOL_NAME(_end);

#define ROOT_TASK_NAME "troot"
#define ROOT_TASK_STACK_SIZE 0x1000
#define ROOT_TASK_PRI 0x1

#ifdef IP_STACK_LWIP
extern void lwip_sys_init();
#endif

void tty_pc_init();

#ifdef INCLUDE_USER_APP_INIT
extern void user_app_init();
#else
inline unsigned long get_CR0(void)
{unsigned long r; __asm__ __volatile__ ("movl %%cr0,%0" : "=q" (r)); return(r);}


static void user_app_init()
{

}
#endif /* INCLUDE_USER_APP_INIT */

static void root_task(void *p)
{
	kprintf("  Root task started...\n");
	tty_init();
	keyboard_init();
	tty_pc_init();
	kprintf("  TTY subsystem inited.\n");

	command_init();
	kprintf("  Shell subsystem inited.\n");

	pci_init();
	//rtl8139_init();
	//c509_init();

	#ifdef INCLUDE_GDB_STUB
	gdb_stub_init();
	kprintf("  gdb stub inited.\n");
	#endif /* INCLUDE_GDB_STUB */

	/* a temp wrapper */
	net_task_init();
	//lwip_sys_init();
	//kprintf("  LWIP inited.\n");

	user_app_init();

	kprintf("  Root task ended.\n");
}

static void root_task_init()
{
	task_t t;

	t = task_create(ROOT_TASK_NAME, root_task, NULL, NULL, ROOT_TASK_STACK_SIZE, 
		ROOT_TASK_PRI, 0, 0);
	assert(t != RERROR);
	
	task_resume_noschedule(t);
}

static void esr_fpu_func(int n)
{
	clts();
}

/* 
 * System startup code, the first entry of C
 * The initial order should not be changed.
 */
void startup(unsigned long magic, unsigned long addr)
{
	int i;
	void *b;

	clear();
	kprintf("\n\nDooloo Operating System Version %s\n", DOOLOO_VERSION);
	kprintf("Author: Puhui Xiong\n\n");

	/* clear bss segment */
//	kprintf("__bss_start=0x%x, _end=0x%x\n", &SYMBOL_NAME(__bss_start), &SYMBOL_NAME(_end));
	memset(&SYMBOL_NAME(__bss_start), 0, &SYMBOL_NAME(_end)-&SYMBOL_NAME(__bss_start));

	/* init exception and interrupt service handle table */
	esr_init();
	isr_init();

	/* Insert the Exceptions handler into IDT */
	IDT_place(0x00,exc0);
	IDT_place(0x01,exc1);
	IDT_place(0x02,exc2);
	IDT_place(0x03,exc3);
	IDT_place(0x04,exc4);
	IDT_place(0x05,exc5);
	IDT_place(0x06,exc6);
	IDT_place(0x07,exc7);
	IDT_place(0x08,exc8);
	IDT_place(0x09,exc9);
	IDT_place(0x0A,exc10);
	IDT_place(0x0B,exc11);
	IDT_place(0x0C,exc12);
	IDT_place(0x0D,exc13);
	IDT_place(0x0E,exc14);
	IDT_place(0x0F,exc15);
	IDT_place(0x10,exc16);

	/* Insert the Interrupt handler into IDT */
	IDT_place(0x40,h0);
	IDT_place(0x41,h1);
	IDT_place(0x42,h2);
	IDT_place(0x43,h3);
	IDT_place(0x44,h4);
	IDT_place(0x45,h5);
	IDT_place(0x46,h6);
	IDT_place(0x47,h7);
	IDT_place(0x70,h8);
	IDT_place(0x71,h9);
	IDT_place(0x72,h10);
	IDT_place(0x73,h11);
	IDT_place(0x74,h12);
	IDT_place(0x75,h13);
	IDT_place(0x76,h14);
	IDT_place(0x77,h15);

	/* init tss descriptor in gdt */
	for (i = 0; i < TSSMAX; i++) {
		b = (void *)(&TSS_table[i]);
		set_TSS(i, (DWORD)b, sizeof(TSS));
	}
	
//	kprintf("PIC_init\n");
	PIC_init();

	/* init clock hardware */
	pit_init(0, TMR_MD2, PIT_FREQENCY/TICKS_PER_SECOND);

	set_TR(TSS_BASE*8);

	/* set FPU exception 7 handler */
	bsp_esr_attach(7, esr_fpu_func);

	/* please don't change the following order */
	memory_inita();

	clock_init(INT_CLOCK);
	kprintf("  Clock subsystem inited.\n");

	task_init();

	ipc_init();
	kprintf("  IPC subsystem inited.\n");

	memory_initb();
	kprintf("  Memory subsystem inited.\n");

	idletask_init();
	root_task_init();
	kprintf("  Task subsystem inited.\n");

	current = &systask[t_idletask];
	kprintf("  Ready to first scheduling, go!!\n");
	task_schedule();

	kprintf("\nSystem halted.\n");
}

