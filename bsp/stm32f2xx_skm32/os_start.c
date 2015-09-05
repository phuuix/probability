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

#include "stm32f2xx.h"
#include "stm32f2xx_tim.h"
#include "stm32f2xx_rcc.h"
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
extern void  systick_init (void);
extern void interrupt_init();
extern void kservice_init();
extern void lwip_sys_init();
extern void lwip_perf_init();
extern void ETH_BSP_Config(void);
extern void http_server_netconn_init();
extern void usbh_cdc_init();

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

	/* loop for 30 ticks */
	while(sys_passed_ticks <= tick+30)
		weight ++;

	loops_per_us = weight/(30*(1000000/TICKS_PER_SECOND))+1;
	kprintf("  udelay: loops per us is %d.\n", loops_per_us);
}

static void root_task(void *p)
{
	//kprintf("  Root task started...\n");

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

	usbh_cdc_init();
	
	#ifdef INCLUDE_NETWORK
	net_task_init();
	ETH_BSP_Config();  // ETH base address: 0x40028000
	lwip_sys_init();
	//lwip_perf_init();
	http_server_netconn_init();
	kprintf("  net inited.\n");
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

/* time counter init: for high solution time (bsp_gettime)
 * timer resolution is 1us, period is 1s
 */
void TIM_init()
{
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	uint16_t PrescalerValue;
	#define TIMFREQ (1000000000/1000)
	
	/* TIM2 clock enable */
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

	/* Compute the prescaler value */
	PrescalerValue = (uint16_t) ((SystemCoreClock/2) / TIMFREQ) - 1;

	/* Time base configuration */
	TIM_TimeBaseStructure.TIM_Period = TIMFREQ - 1;
	TIM_TimeBaseStructure.TIM_Prescaler = 0;
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;

	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);

	/* Prescaler configuration */
	TIM_PrescalerConfig(TIM2, PrescalerValue, TIM_PSCReloadMode_Immediate);

	/* TIM2 enable counter */
	TIM_Cmd(TIM2, ENABLE);
}

/* hardware init before os is ready */
void os_hw_init()
{
	/* init USART */
	uart_init(SERIAL_UART_PORT, SERIAL_UART_BAUDRATE);

	/* init interrupt related matters */
	interrupt_init();

	TIM_init();
}

// TODO: To standard this scenario
void start_dooloo(void)
{
	bsp_int_disable();

	os_hw_init();

	kprintf("\n\nProbability kernel: version %s\n", DOOLOO_VERSION);
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

