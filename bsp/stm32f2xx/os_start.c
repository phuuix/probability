/*
 * the first C file of operating system.
 */
#include <config.h>
#include <version.h>
#include <bsp.h>
#include <task.h>
#include <lock.h>
#include <ipc.h>
#include <assert.h>
#include <clock.h>
#include <idletask.h>
#include <s_alloc.h>
#include <command.h>
#include <memory.h>
#include <string.h>
#include <stdlib.h>

#include "stm32f2xx.h"
#include "stm32f2xx_hal.h"
#include "journal.h"
#include "uart.h"
#include "uprintf.h"
#include "net/net_task.h"

char _irq_stack_start[1024];


#define ROOT_TASK_NAME "troot"
#define ROOT_TASK_STACK_SIZE 0x400
#define ROOT_TASK_PRI 0x1

extern void console_init();
extern void samv_agent_init();
extern void systick_init (void);
extern void interrupt_init();
extern void kservice_init();
extern void lwip_sys_init();
extern void http_server_netconn_init();
extern void cc2530ctrl_init();
extern void bsp_gettime_init();
extern uint32_t udelay_init();

#ifdef INCLUDE_USER_APP_INIT
extern void user_app_init();
#else

static void user_app_init()
{
	
}
#endif /* INCLUDE_USER_APP_INIT */


static void root_task(void *p)
{
	clock_init(INT_SYSTICK);
	systick_init();
	kprintf("  Clock subsystem inited.\n");

	uprintf_init();
	kprintf("  Uprintf task started.\n");
	
	console_init();
	kprintf("  Console subsystem inited.\n");

#ifdef INLCUDE_KSERV
	kservice_init();
	kprintf("  kservice task started.\n");
#endif

#ifdef INLCUDE_COMMAND
	command_init();
	kprintf("  Shell subsystem inited.\n");
#endif

	#ifdef INCLUDE_GDB_STUB
	gdb_stub_init();
	kprintf("  gdb stub inited.\n");
	#endif /* INCLUDE_GDB_STUB */


	// old usbh conflict with RLC8021 pin assignment
	cc2530ctrl_init();
	kprintf("  cc2530 task inited.\n");

	#ifdef INCLUDE_NETWORK
	//net_task_init();
	// ETH base address: 0x40028000
	lwip_sys_init();
	kprintf("  LWIP inited.\n");
	http_server_netconn_init();
	kprintf("  HTTP server inited.\n");
	#endif

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


static void default_sched_hook(struct dtask *from, struct dtask *to)
{
	uint8_t *ptr_stack;
	
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

/* hardware init before os is ready */
void os_hw_init()
{
	/* init USART */
	uart_init_console(SERIAL_UART_PORT, SERIAL_UART_BAUDRATE, 0);

	/* init interrupt related matters */
	interrupt_init();

	bsp_gettime_init();
}

// TODO: To standard this scenario
void start_dooloo(void)
{
	bsp_int_disable();

	os_hw_init();

	kprintf("\n\nProbability kernel: version %s\n", OS_VERSION);
	kprintf("Author: Puhui Xiong (bearix@hotmail.com) \n\n");

	/* trace irq stack */
	_irq_stack_start[0] = _irq_stack_start[1] = '#';
	
	memory_inita();
	
	task_init();
	kprintf("  Task subsystem inited.\n");
	
	ipc_init();
	kprintf("  IPC subsystem inited.\n");

	memory_initb();
	kprintf("  Memory subsystem inited.\n");
	
	idletask_init();
	root_task_init();
	kprintf("  Create task tidle and troot.\n");

	task_set_schedule_hook(default_sched_hook);
	
	/* set current to an invalid tcb, then begin to schedule */
	current = &systask[0];
	kprintf("  Ready to first scheduling, go!!\n\n");
	/* before schedule, we have to enable interrupt for pendsv() used */
	bsp_int_enable();
	task_schedule();

	kprintf("\nSystem halted.\n");
	while(1);
}

