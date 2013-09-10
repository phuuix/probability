/*
 * os_support_c.c
 * OS support C functions for 
 */
#include <bsp.h>
#include <config.h>
#include <task.h>
#include <memory.h>
#include <ucontext.h>

#include "siprintf.h"
#include "probability.h"


#define MAX_HANDLERS	64


/* the two variables are used in bsp_task_switch_interrupt() function */
uint32_t interrupt_from_task, interrupt_to_task;
static uint32_t isr_table[MAX_HANDLERS];

/* dummy interrupt service function */
static void dummy_isr_fun(int i);
extern char _irq_stack_start[];
extern struct dtask systask[];

uint32_t fsave()
{
	// mask signal
	return 0;
}

void frestore(uint32_t flags)
{
	// unmask signal ?
}

void bsp_task_switch(uint32_t from, uint32_t to)
{
	swapcontext(&uthread_slots[from].context, &uthread_slots[to].context);
}

void bsp_task_switch_interrupt(uint32_t from, uint32_t to)
{
	bsp_task_switch(from, to);
}

void dump_buffer(uint8_t *buffer, uint16_t length)
{
	uint16_t i;

	for(i=0; i<length; i++)
	{
		kprintf("%02x ", buffer[i]);
		if((i+1)%16 == 0)
			kprintf("\n");
		else if((i+1)%8 == 0)
			kprintf("-- ");
	}
	kprintf("\n");
}

/******************************************************
 * support of host simulation from ucontext: start
 ******************************************************/
struct uthread_struct
{
	ucontext_t context;
	void (*func)(void *arg);
	void *arg;
	void (*killer)(void);
	char used;
};

static struct uthread_struct uthread_slots[MAX_TASK_NUMBER];

static void uthread_helper(int tid)
{
	uthread_slots[tid].func( uthread_slots[tid].arg);
	uthread_slots[tid].used = 0;
	uthread_slots[tid].killer();
}
/******************************************************
 * support of host simulation from ucontext: end
 ******************************************************/

/* Init registers stacked as if auto-saved on exception */
char *bsp_task_init(int tid, void (*task)(void *p), void *parm,
	char *stack, void (*killer)(void))
{
	assert(uthread_slots[tid].used == 0);
	
	getcontext(&uthread_slots[tid].context);
	uthread_slots[tid].context.uc_stack.ss_sp = stack;
	uthread_slots[tid].context.uc_stack.ss_size = ;
	uthread_slots[tid].context.uc_link = NULL;	// make successor context NULL
	
	uthread_slots[tid].func = task;
	uthread_slots[tid].arg =parm;
	uthread_slots[tid].killer = killer;
	uthread_slots[tid].used = 1;
	makecontext(&uthread_slots[i].context, uthread_helper, 1, tid);
	
	/* return task's current stack address */
	return (char *)stk;
}

uint32_t get_fp(void)
{
    return 0;
}

/* disable irq
 * irq: exception number
 */
void irq_mask(uint32_t irqno)
{
}

/* enable irq
 * irq: exception number
 */
void irq_unmask(uint32_t irqno)
{
}

/* attach isr
 * index: exception number
 */
void isr_attach(uint16_t index, void (*handler)(int n))
{
	if(index < MAX_HANDLERS)
	{
		isr_table[index] = (uint32_t)handler;
	}
}


/* deattach isr
 * index: exception number
 */
void isr_detach(uint16_t index)
{
	if(index < MAX_HANDLERS)
		isr_table[index] = (uint32_t)dummy_isr_fun;
}


void esr_attach(uint16_t index, void (*handler)(int n))
{
}

void esr_detach(uint16_t index)
{
}

/* enable IRQ/FIQ interrupts */
void interrupt_enable(void)
{
}


/* 
 * disable IRQ/FIQ interrupts
 * returns true if interrupts had been enabled before we disabled them
 */
int interrupt_disable(void)
{
	return 0;
}

/* dummy interrupt service function */
static void dummy_isr_fun(int i)
{
	kprintf("Unhandled interrupt %d occured!!!\n", i);

    panic("Unhandled interrupt\n");
	
	while(1);
}


typedef void (*ISR_FUNC)(int);

void interrupt_init()
{

}

void isr_default_handler()
{

}


/*
 * fp: top of innerest stack
 * low: low value of stack
 * high: high vlaue of stack
 */
void dump_call_stack(uuint32_t fp, uint32_t low, uint32_t high)
{
#if 0 // not valid in cortex, only can be used in arm7/9
	uint32_t *pStack, lr, loops = 0;

	kprintf("calling stacks:\n");
	while(fp>=low && fp<=high && loops <20)
	{
		/* {fp, ip, lr, pc} */
		pStack = (uint32_t *)(fp-12);
		//pc = pStack[3];
		lr = pStack[2];
		//ip = pStack[1];
		fp = pStack[0];
		kprintf("%d -- 0x%x \n", loops, lr);
		loops ++;
	}
#endif
}

/* stack dump and information dump
 * for stack dump, a common rules is:
 * 0x2000xxxx (r7) 0x0000xxxx(lr), r7 will in stack and lr will in flash.
 * usually 12th long word is the address which calls panic().
 */
void panic(char *infostr)
{
	uint32_t sp;
	uint32_t size;
	fsave();
	
	kprintf("PANIC: %s\n", infostr);
#if 0
	if(get_psr() & 0xFF){
		/* in exception context, dump exception stack */
		sp = __get_MSP();
		if((sp>(uint32_t)_irq_stack_start) && (sp<(uint32_t)_irq_stack_start+1024))
		{
			size = (uint32_t)_irq_stack_start+1024-sp;
			kprintf("exception stacks: sp=0x%x depth=%d bytes\n", sp, size);
			dump_buffer((uint8_t *)sp, size);
		}
		else
			kprintf("broken MSP: 0x%x\n", sp);
	}

	
	if((current>=&systask[0]) && (current<&systask[MAX_TASK_NUMBER]))
	{
		/* dump task stack */
		sp = __get_PSP();
		if((sp>(uint32_t)current->stack_base) && (sp<(uint32_t)current->stack_base+current->stack_size))
		{
			size = (uint32_t)current->stack_base+current->stack_size-sp;
			kprintf("task stacks: sp=0x%x depth=%d bytes\n", sp, size);
			dump_buffer((uint8_t *)sp, size);
		}
		else
			kprintf("broken PSP: 0x%x\n", sp);

		/* dump current task info */
		kprintf("current=0x%x last sp=0x%x stack_base=0x%x taskname=%s state=%d taskq=0x%x\n", 
			current, current->sp, current->stack_base, current->name, current->state, current->taskq);
	}
	else
		kprintf("current is overwriten! current=0x%x\n", current);
#endif // 0

	/* dump system ready queue */

	/* dump memory usage */
	memory_dump();
	
    while(1);
}

void  systick_init (void)
{
    uint32_t  cnts;
	RCC_ClocksTypeDef  rcc_clocks;

    RCC_GetClocksFreq(&rcc_clocks);

    cnts = rcc_clocks.HCLK_Frequency / TICKS_PER_SECOND;

	SysTick->LOAD = (cnts - 1);
	SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk;
}

int kprint_outchar(printdev_t *dev, char c)
{
	putc(c);
	return 1;
}


