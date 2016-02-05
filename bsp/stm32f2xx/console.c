/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
 *
 * Copyright (c) Puhui Xiong <phuuix@163.com>
 * @file
 *   console.c
 *
 * @History
 *   AUTHOR		 DATE			NOTES
*/

#include <config.h>
#include <bsp.h>
#include <task.h>
#include <assert.h>
#include <ipc.h>

#include "uart.h"
#include "cli.h"
#include "stm32f2xx.h"
#include "stm32f2xx_hal.h"

int getline(char *buf, int len);

#define TCONSOLE_STACK_SIZE 0x400
#define TCONSOLE_PRIORITY 0x3
#define INT_CONSOLE 39+16   //USART3

#define CBUF_SIZE 32
static mbox_t console_mbox;
static uint32_t console_cbuf[CBUF_SIZE*sizeof(uint32_t)];

extern UART_HandleTypeDef UartHandle[2];

static void serial_isr_func(int n)
{
	uint32_t tmp1 = 0;
	UART_HandleTypeDef *huart = &UartHandle[SERIAL_UART_PORT];
	uint32_t buf[16];
	uint32_t nBytes = 0;

	if(huart->Instance->SR & (UART_FLAG_PE | UART_FLAG_FE | UART_FLAG_NE | UART_FLAG_ORE))
	{
		__HAL_UART_CLEAR_OREFLAG(huart);
	}

	while((__HAL_UART_GET_FLAG(huart, UART_FLAG_RXNE) == SET) && (nBytes < 16))
	{
		buf[nBytes ++] = (uint8_t)(huart->Instance->DR & (uint8_t)0x00FF);
	}
	
	for(tmp1=0; tmp1<nBytes; tmp1++)
	{
		mbox_post(&console_mbox, &buf[tmp1]);
	}

}

static void console_task(void *p)
{
	char buf[100];

	mbox_initialize(&console_mbox, 1, CBUF_SIZE, console_cbuf);
	
	bsp_isr_attach(INT_CONSOLE, serial_isr_func);
	bsp_irq_unmask(INT_CONSOLE);
	
	kprintf("Bayes# ");

	while(1)
	{
		if(getline(buf, 100))
			cmd_process(buf);
		kprintf("Bayes# ");
	}
}

void console_init()
{
	task_t tt;

	tt = task_create("tconsole", console_task, NULL, NULL, TCONSOLE_STACK_SIZE, TCONSOLE_PRIORITY, 20, 0);

	assert(tt != -1);
	task_resume_noschedule(tt);

}

/*
 * get a char
 *   return: char on success; 0 on failed
 */
char getcharacter()
{
	uint32_t e = 0;
	
	mbox_pend(&console_mbox, &e, TIME_WAIT_FOREVER);

	return e;
}


/*
 * get a line
 *   buf: receive buffer
 *   len: buffer length
 *   return: line length on success; 0 on failed
 */
int getline(char *buf, int len)
{
	int n = 0, term = 0;
	char c;

	do{
		c=getcharacter();
		//kprintf("rx char: %c (%d)\n", c, c);
		if(c == '\b')	//backspace
		{
			if(n>0)
			{
				uart_putc_ex(SERIAL_UART_PORT, '\b');
				uart_putc_ex(SERIAL_UART_PORT, ' ');
				uart_putc_ex(SERIAL_UART_PORT, '\b');
				n--;
			}
		}
		else if(c == '\n' || c == '\r')
		{
			uart_putc_ex(SERIAL_UART_PORT, '\n');
			term = 1;
		}
		else{
			buf[n++] = c;
			uart_putc_ex(SERIAL_UART_PORT, c);
		}
	}
	while(term==0 && n<len);
	
	buf[n] = '\0';

	return n;
}

