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
 *   uart.c
 *
 * @History
 *   AUTHOR	  DATE			 NOTES
 *   puhuix   20160109       disable interrupt on HAL_UART_Receive and HAL_UART_Transmit
*/

#include <bsp.h>
#include <assert.h>

#include "stm32f2xx.h"
#include "stm32f2xx_hal.h"
#include "uart.h"

#define COMn 2

UART_HandleTypeDef UartHandle[2];

/* COM1 (USART3) for debug trace
 * COM2 (USART6) connect to CC2530 USART0
 */
/* console: USART3, GPIOC, TX Pin10, RX Pin11 */
void uart_init_console(uint32_t port, uint32_t baudrate, uint32_t flowctrl)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	/* Enable GPIO clock for Rx and Tx pin */
	__HAL_RCC_GPIOC_CLK_ENABLE();

	/* Enable UART clock */
	__HAL_RCC_USART3_CLK_ENABLE();
	
	/* Configure USART Tx as alternate function  */
	GPIO_InitStructure.Pin = GPIO_PIN_10;
	GPIO_InitStructure.Mode = GPIO_MODE_AF_PP;
	GPIO_InitStructure.Pull = GPIO_PULLUP;
	GPIO_InitStructure.Speed = GPIO_SPEED_FAST;
	GPIO_InitStructure.Alternate = GPIO_AF7_USART3;
	HAL_GPIO_Init(GPIOC, &GPIO_InitStructure);
	
	/* Configure USART Rx as alternate function  */
	GPIO_InitStructure.Pin = GPIO_PIN_11;
	GPIO_InitStructure.Alternate = GPIO_AF7_USART3;
	HAL_GPIO_Init(GPIOC, &GPIO_InitStructure);

	/*##-1- Configure the UART peripheral ######################################*/
	UartHandle[port].Instance          = USART3;

	UartHandle[port].Init.BaudRate     = baudrate;
	UartHandle[port].Init.WordLength   = UART_WORDLENGTH_8B;
	UartHandle[port].Init.StopBits     = UART_STOPBITS_1;
	UartHandle[port].Init.Parity       = USART_PARITY_NONE;
	UartHandle[port].Init.HwFlowCtl    = flowctrl;
	UartHandle[port].Init.Mode         = UART_MODE_TX_RX;
	UartHandle[port].Init.OverSampling = UART_OVERSAMPLING_16;
    
  	HAL_UART_Init(&UartHandle[port]);

	/* Enable the UART Data Register not empty Interrupt */
    __HAL_UART_ENABLE_IT(&UartHandle[port], UART_IT_RXNE);
	__HAL_UART_ENABLE_IT(&UartHandle[port], UART_IT_PE);
	__HAL_UART_ENABLE_IT(&UartHandle[port], UART_IT_ERR);
}

/* CC2530: USART6, GPIOC, TX Pin6, RX Pin7 */
void uart_init_cc2530(uint32_t port, uint32_t baudrate, uint32_t flowctrl)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	/* Enable GPIO clock for Rx and Tx pin */
	__HAL_RCC_GPIOC_CLK_ENABLE();

	/* Enable UART clock */
	__HAL_RCC_USART6_CLK_ENABLE();
	
	/* Configure USART Tx as alternate function  */
	GPIO_InitStructure.Pin = GPIO_PIN_6;
	GPIO_InitStructure.Mode = GPIO_MODE_AF_PP;
	GPIO_InitStructure.Pull = GPIO_PULLUP;
	GPIO_InitStructure.Speed = GPIO_SPEED_FAST;
	GPIO_InitStructure.Alternate = GPIO_AF8_USART6;
	HAL_GPIO_Init(GPIOC, &GPIO_InitStructure);
	
	/* Configure USART Rx as alternate function  */
	GPIO_InitStructure.Pin = GPIO_PIN_7;
	GPIO_InitStructure.Alternate = GPIO_AF8_USART6;
	HAL_GPIO_Init(GPIOC, &GPIO_InitStructure);

	/*##-1- Configure the UART peripheral ######################################*/
	UartHandle[port].Instance          = USART6;

	UartHandle[port].Init.BaudRate     = baudrate;
	UartHandle[port].Init.WordLength   = UART_WORDLENGTH_8B;
	UartHandle[port].Init.StopBits     = UART_STOPBITS_1;
	UartHandle[port].Init.Parity       = USART_PARITY_NONE;
	UartHandle[port].Init.HwFlowCtl    = flowctrl;
	UartHandle[port].Init.Mode         = UART_MODE_TX_RX;
	UartHandle[port].Init.OverSampling = UART_OVERSAMPLING_16;
    
  	HAL_UART_Init(&UartHandle[port]);

	/* Enable the UART Data Register not empty Interrupt */
    __HAL_UART_ENABLE_IT(&UartHandle[port], UART_IT_RXNE);
	__HAL_UART_ENABLE_IT(&UartHandle[port], UART_IT_PE);
	__HAL_UART_ENABLE_IT(&UartHandle[port], UART_IT_ERR);
}

uint16_t uart_getc(uint8_t COM)
{
	uint8_t c = 0xEE;
	uint32_t flags;
	UART_HandleTypeDef *huart = &UartHandle[COM];

	if(COM < COMn)
	{
		flags = bsp_fsave();
		while(__HAL_UART_GET_FLAG(huart, UART_FLAG_RXNE) == RESET);
		c = (uint8_t)(huart->Instance->DR & (uint8_t)0x00FF);
		bsp_frestore(flags);
	}
	

	return c;
}

void uart_putc(uint8_t COM, uint8_t c)
{
	int uart_status;
	uint32_t flags;
	
	if(COM < COMn)
	{
		flags = bsp_fsave();
		uart_status = HAL_UART_Transmit(&UartHandle[COM], &c, 1, HAL_MAX_DELAY);
		assert(uart_status == HAL_OK);
		bsp_frestore(flags);
	}
}


void uart_putc_ex(uint8_t COM, uint8_t c)
{
	uint32_t flags;
	
	if(COM < COMn)
	{
		uint8_t c_return = '\r';

		flags = bsp_fsave();
		/* \n ==> \r\n */
		if(c == '\n'){
			HAL_UART_Transmit(&UartHandle[COM], &c_return, 1, HAL_MAX_DELAY);
		}
	    HAL_UART_Transmit(&UartHandle[COM], &c, 1, HAL_MAX_DELAY);
		bsp_frestore(flags);
	}
}



