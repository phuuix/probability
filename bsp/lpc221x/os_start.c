/*
 * the first C file of operating system.
 */
#include <config.h>
#include <version.h>
#include <bsp.h>
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

#include "lpc2xxx.h"
#include "serial.h"
#include "interrupts.h"
#include "journal.h"

char _irq_stack_start[768];
char _abort_stack_start[64];

extern unsigned char __bss_start;
extern unsigned char __bss_end;

#define ROOT_TASK_NAME "troot"
#define ROOT_TASK_STACK_SIZE 0x300
#define ROOT_TASK_PRI 0x1

extern void console_init();
extern void kservice_init();
extern void samv_agent_init();

#ifdef INCLUDE_USER_APP_INIT
extern void user_app_init();
#else

static void user_app_init()
{
}
#endif /* INCLUDE_USER_APP_INIT */


/* one solution for udelay */
extern uint32_t sys_passed_ticks;
static uint32_t loops_per_us = 0;
void udelay(uint32_t us)
{
	uint32_t loops;

	for(loops=0; loops<us*(loops_per_us+1); loops++);
}

void udelay_init()
{
	uint32_t tick = sys_passed_ticks;
	uint32_t weight = 0;

	while(tick == sys_passed_ticks);

	/* loop for 10 ticks */
	while(sys_passed_ticks <= tick+30)
		weight ++;

	loops_per_us = weight/(30*(1000000/TICKS_PER_SECOND))+1;
	kprintf("  udelay: loops_per_us=%d weight=%d\n", loops_per_us, weight);
}

static void root_task(void *p)
{
	kprintf("  Root task started...\n");
	
	console_init();
	kprintf("  Console subsystem inited.\n");

//	kservice_init();
//	kprintf("  tkservice started.\n");
//	command_init();
//	kprintf("  Shell subsystem inited.\n");

	samv_agent_init();
	kprintf("  Samv agent inited.\n");

	#ifdef INCLUDE_GDB_STUB
//	gdb_stub_init();
//	kprintf("  gdb stub inited.\n");
	#endif /* INCLUDE_GDB_STUB */

	/* a temp wrapper */
//	net_task_init();
//	lwip_sys_init();
//	kprintf("  LWIP inited.\n");

	udelay_init();
	
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


/* clear .bss */
void clear_bss()
{
	unsigned char *dst;

	dst = &__bss_start; 
	while (dst < &__bss_end)
		*dst++ = 0;
}

uint32_t g_trace_flags;
static void default_sched_hook(struct dtask *from, struct dtask *to)
{
	uint8_t *ptr_stack;

	/* update sched history */
	journal_task_switch(from, to);
	
	/* check stack overflow: if task is dead, we don't check it */
	if(from->state != TASK_STATE_DEAD)
	{
		ptr_stack = (uint8_t *)from->stack_base;
				
		if(*ptr_stack != TASK_STACK_INIT_CHAR)
		{
			kprintf("error: task %s stack overflow!!\n", from->name);
			dump_call_stack(get_fp(), MEMLOW, MEMHIGH);
			panic("stack overflow\n");
		}
	}

	/* update consumed time for from task */

	
}

void start_dooloo(void)
{
	bsp_fsave();
	
	lpc221x_init();
	
	serial_init();

	clear_bss();
	/* trace irq stack */
	_irq_stack_start[0] = _irq_stack_start[1] = '#';

	kprintf("\n\nDooloo Operating System Version %s\n", DOOLOO_VERSION);
	kprintf("Author: Puhui Xiong (bearix@hotmail.com) \n\n");

	memory_inita();
	
	clock_init(INTSRC_TIMER0);
	kprintf("  Clock subsystem inited.\n");

	task_init();
	kprintf("  Task subsystem inited.\n");
	
	ipc_init();
	kprintf("  IPC subsystem inited.\n");

	memory_initb();
	kprintf("  Memory subsystem inited.\n");
	
	idletask_init();
	root_task_init();
	kprintf("  Create task tidle and troot.\n");

	//interrupt_enable();

	task_set_schedule_hook(default_sched_hook);
	
	/* set current to an invalid tcb, then begin to schedule */
	current = &systask[0];
	kprintf("  Ready to first scheduling, go!!\n\n");
	task_schedule();

	kprintf("\nSystem halted.\n");
	while(1);
}

