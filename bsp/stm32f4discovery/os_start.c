/*
 * the first C file of operating system.
 */
#include <config.h>
#include <stdlib.h>
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

#include "stm32f4xx.h"
#include "stm32f4xx_tim.h"
#include "stm32f4xx_rcc.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4_discovery_lis302dl.h"
#include "journal.h"
#include "uart.h"
#include "probability.h"

char _irq_stack_start[1024];


#define ROOT_TASK_NAME "troot"
#define ROOT_TASK_STACK_SIZE 0x400
#define ROOT_TASK_PRI 0x1

extern void console_init();
extern void samv_agent_init();
extern void  systick_init (void);
extern void interrupt_init();
extern void kservice_init();

#ifdef INCLUDE_USER_APP_INIT
extern void user_app_init();
#else
uint32_t LIS302DL_TIMEOUT_UserCallback(void)
{
  /* Block communication and all processes */
  kprintf("LIS302DL_TIMEOUT!\n");
  while (1)
  {   
  }
}

static void mems_init(uint8_t *calibration)
{
	uint8_t ctrl = 0;
	
	LIS302DL_InitTypeDef  LIS302DL_InitStruct;
	LIS302DL_InterruptConfigTypeDef LIS302DL_InterruptStruct;  

	/* SysTick end of count event each 10ms */
	// SysTick_Config(SystemCoreClock/ 100);

	/* Set configuration of LIS302DL*/
	LIS302DL_InitStruct.Power_Mode = LIS302DL_LOWPOWERMODE_ACTIVE;
	LIS302DL_InitStruct.Output_DataRate = LIS302DL_DATARATE_100;
	LIS302DL_InitStruct.Axes_Enable = LIS302DL_X_ENABLE | LIS302DL_Y_ENABLE | LIS302DL_Z_ENABLE;
	LIS302DL_InitStruct.Full_Scale = LIS302DL_FULLSCALE_2_3;
	LIS302DL_InitStruct.Self_Test = LIS302DL_SELFTEST_NORMAL;
	LIS302DL_Init(&LIS302DL_InitStruct);

	/* Set configuration of Internal High Pass Filter of LIS302DL*/
	LIS302DL_InterruptStruct.Latch_Request = LIS302DL_INTERRUPTREQUEST_LATCHED;
	LIS302DL_InterruptStruct.SingleClick_Axes = LIS302DL_CLICKINTERRUPT_Z_ENABLE;
	LIS302DL_InterruptStruct.DoubleClick_Axes = LIS302DL_DOUBLECLICKINTERRUPT_Z_ENABLE;
	LIS302DL_InterruptConfig(&LIS302DL_InterruptStruct);

	/* Required delay for the MEMS Accelerometre: Turn-on time = 3/Output data Rate 
	                                                         = 3/100 = 30ms */
	task_delay(3);

	/* Configure Interrupt control register: enable Click interrupt1 */
	ctrl = 0x07;
	LIS302DL_Write(&ctrl, LIS302DL_CTRL_REG3_ADDR, 1);

	/* Enable Interrupt generation on click/double click on Z axis */
	ctrl = 0x70;
	LIS302DL_Write(&ctrl, LIS302DL_CLICK_CFG_REG_ADDR, 1);

	/* Configure Click Threshold on X/Y axis (10 x 0.5g) */
	ctrl = 0xAA;
	LIS302DL_Write(&ctrl, LIS302DL_CLICK_THSY_X_REG_ADDR, 1);

	/* Configure Click Threshold on Z axis (10 x 0.5g) */
	ctrl = 0x0A;
	LIS302DL_Write(&ctrl, LIS302DL_CLICK_THSZ_REG_ADDR, 1);

	/* Configure Time Limit */
	ctrl = 0x03;
	LIS302DL_Write(&ctrl, LIS302DL_CLICK_TIMELIMIT_REG_ADDR, 1);

	/* Configure Latency */
	ctrl = 0x7F;
	LIS302DL_Write(&ctrl, LIS302DL_CLICK_LATENCY_REG_ADDR, 1);

	/* Configure Click Window */
	ctrl = 0x7F;
	LIS302DL_Write(&ctrl, LIS302DL_CLICK_WINDOW_REG_ADDR, 1);

	/* return calibration values */
	LIS302DL_Read(calibration, LIS302DL_OUT_X_ADDR, 6);
}

void led_gpio_init()
{
	GPIO_InitTypeDef GPIO_InitStructure;

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);
	
	/* Configure PD12, PD13, PD14 and PD15 in output pushpull mode */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12 | GPIO_Pin_13| GPIO_Pin_14| GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOD, &GPIO_InitStructure);
}

static void tled_main(void *param)
{
	uint32_t now, last = 0;
	uint8_t led[4] = {0};
	uint8_t buffer[6], offset[6];

	/* gpio init */
	led_gpio_init();

	/* init mems */
	mems_init(offset);
	
	while(1)
	{
		task_delay(1);
		now = tick();
		if(now - last > 50)
		{
			/* every 320 ms, toggle led0 */
			GPIO_WriteBit(GPIOD, GPIO_Pin_12, led[0]);
			led[0] = (led[0] == 0);
			last = now;
		}

		LIS302DL_Read(buffer, LIS302DL_OUT_X_ADDR, 6);

		if(abs(buffer[0] - offset[0]) > 2)
		{
			GPIO_SetBits(GPIOD, GPIO_Pin_13);
		}
		else
			GPIO_ResetBits(GPIOD, GPIO_Pin_13);

		if(abs(buffer[2] - offset[2]) > 2)
		{
			GPIO_SetBits(GPIOD, GPIO_Pin_14);
		}
		else
			GPIO_ResetBits(GPIOD, GPIO_Pin_14);

		if(abs(buffer[4] - offset[4]) > 2)
		{
			GPIO_SetBits(GPIOD, GPIO_Pin_15);
		}
		else
			GPIO_ResetBits(GPIOD, GPIO_Pin_15);
		
		/* PD12 to be toggled 
    		GPIO_SetBits(GPIOD, GPIO_Pin_12);
		
		task_delay(50);

		GPIO_ResetBits(GPIOD, GPIO_Pin_12);

		task_delay(50); 
		*/
	}
}

static void user_app_init()
{
	task_t tt;
	
	/* start kservice task */
	tt = task_create("tled", tled_main, NULL, NULL, 0x400, 256-64, 0, 0);
	task_resume_noschedule(tt);
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
	kprintf("  Root task started...\n");

	clock_init(INT_SYSTICK);
	systick_init();
	kprintf("  Clock subsystem inited.\n");
	
	console_init();
	kprintf("  Console subsystem inited.\n");

//	command_init();
//	kprintf("  Shell subsystem inited.\n");


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

/* hardware init before os is ready */
void os_hw_init()
{
	/* init USART */
	uart_init(SERIAL_UART_PORT, SERIAL_UART_BAUDRATE);

	/* read back SystemCoreClock */
	SystemCoreClockUpdate();
	
	/* init interrupt related matters */
	interrupt_init();

	/* time counter init: for high solution time */
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	uint16_t PrescalerValue;
	#define TIMFREQ (1000000000/1000)
	
	/* TIM2 clock enable */
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

	/* Compute the prescaler value */
	PrescalerValue = (uint16_t) ((SystemCoreClock) / TIMFREQ) - 1;

	/* Time base configuration */
	TIM_TimeBaseStructure.TIM_Period = TIMFREQ - 1;
	TIM_TimeBaseStructure.TIM_Prescaler = 0;
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;

	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);

	/* Prescaler configuration */
	TIM_PrescalerConfig(TIM2, PrescalerValue, TIM_PSCReloadMode_Immediate);
}

// TODO: To standard this scenario
void start_dooloo(void)
{
	bsp_int_disable();

	os_hw_init();

	kprintf("\n\nDooloo Operating System Version %s\n", OS_VERSION);
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

