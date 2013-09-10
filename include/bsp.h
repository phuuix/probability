/*************************************************************************/
/* The Dooloo kernel                                                     */
/* Copyright (C) 2004-2006 Xiong Puhui (Bearix)                          */
/* All Rights Reserved.                                                  */
/*                                                                       */
/* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS  */
/* THE PROPERTY OF DOOLOO RTOS DEVELOPMENT TEAM                          */
/*                                                                       */
/*************************************************************************/

/*************************************************************************/
/*                                                                       */
/* FILE                                       VERSION                    */
/*   bsp.h                                     0.3.0                     */
/*                                                                       */
/* COMPONENT                                                             */
/*   Kernel                                                              */
/*                                                                       */
/* DESCRIPTION                                                           */
/*   This header file is BSP's interface to Dooloo                       */
/*                                                                       */
/* CHANGELOG                                                             */
/*   AUTHOR         DATE                    NOTES                        */
/*   Bearix         2006-8-20               Version 0.3.0                */
/*************************************************************************/ 

#ifndef __D_BSP_H__
#define __D_BSP_H__

#include "inttypes.h"

/* internal memory range  */
#define MEMLOW 0x20000000
#define MEMHIGH 0x20020000

extern uint32_t sys_passed_ticks;
#define KPRINTF(format, ...) \
do{ \
	uint32_t f = bsp_fsave(); \
	kprintf("%10d: ", sys_passed_ticks); \
	kprintf(format, ##__VA_ARGS__); \
	bsp_frestore(f); \
}while(0)


uint32_t get_fp(void);
void dump_call_stack(uint32_t fp, uint32_t low, uint32_t high);


void panic(char *infostr);

/* kprintf */
int kprintf(char *fmt,...);

/* interrupt and exception interface */
void bsp_int_enable(void);

int bsp_int_disable(void);

uint32_t bsp_fsave(void);

void bsp_frestore(uint32_t flags);

void bsp_irq_mask(uint32_t irqno);

void bsp_irq_unmask(uint32_t irqno);

void bsp_isr_attach(uint16_t index, void (*handler)(int n));

void bsp_isr_detach(uint16_t index);

void bsp_esr_attach(uint16_t index, void (*handler)(int n));

void bsp_esr_detach(uint16_t index);

/* task support interface */
void bsp_task_switch(uint32_t from, uint32_t to);

void bsp_task_switch_interrupt(uint32_t from, uint32_t to);

char *bsp_task_init(int index, void (*task)(void *p), void *parm, char *stack, void (*killer)(void));

/* libc interface */

/* GDB remote debug stub interface */
void breakpoint();

void dump_buffer(uint8_t *buffer, uint16_t length);

/* other hardware and software interface */

#endif /* __D__BSP_H__ */

