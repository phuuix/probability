/*
 * uart.c
 */

#include <bsp.h>

#include "stm32f2xx_usart.h"
#include "stm32f2xx_gpio.h"
#include "stm32f2xx_rcc.h"
#include "uart.h"

#define COMn 1

USART_TypeDef* COM_USART[COMn] = {USART3}; 

GPIO_TypeDef* COM_TX_PORT[COMn] = {GPIOC};
 
GPIO_TypeDef* COM_RX_PORT[COMn] = {GPIOC};

const uint32_t COM_USART_CLK[COMn] = {RCC_APB1Periph_USART3};

const uint32_t COM_TX_PORT_CLK[COMn] = {RCC_AHB1Periph_GPIOC};
 
const uint32_t COM_RX_PORT_CLK[COMn] = {RCC_AHB1Periph_GPIOC};

const uint16_t COM_TX_PIN[COMn] = {GPIO_Pin_10};

const uint16_t COM_RX_PIN[COMn] = {GPIO_Pin_11};
 
const uint8_t COM_TX_PIN_SOURCE[COMn] = {GPIO_PinSource10};

const uint8_t COM_RX_PIN_SOURCE[COMn] = {GPIO_PinSource11};
 
const uint8_t COM_TX_AF[COMn] = {GPIO_AF_USART3};
 
const uint8_t COM_RX_AF[COMn] = {GPIO_AF_USART3};


void uart_init(uint8_t COM, uint32_t baudrate)
{
	USART_InitTypeDef usart_def;

	if(COM < COMn)
	{
		GPIO_InitTypeDef GPIO_InitStructure;

		/* Enable GPIO clock */
		RCC_AHB1PeriphClockCmd(COM_TX_PORT_CLK[COM] | COM_RX_PORT_CLK[COM], ENABLE);

		if (COM == 0)
		{
			/* Enable UART clock */
			RCC_APB1PeriphClockCmd(COM_USART_CLK[COM], ENABLE);
		}

		/* Connect PXx to USARTx_Tx*/
		GPIO_PinAFConfig(COM_TX_PORT[COM], COM_TX_PIN_SOURCE[COM], COM_TX_AF[COM]);

		/* Connect PXx to USARTx_Rx*/
		GPIO_PinAFConfig(COM_RX_PORT[COM], COM_RX_PIN_SOURCE[COM], COM_RX_AF[COM]);

		/* Configure USART Tx as alternate function  */
		GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
		GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;

		GPIO_InitStructure.GPIO_Pin = COM_TX_PIN[COM];
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
		GPIO_Init(COM_TX_PORT[COM], &GPIO_InitStructure);

		/* Configure USART Rx as alternate function  */
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
		GPIO_InitStructure.GPIO_Pin = COM_RX_PIN[COM];
		GPIO_Init(COM_RX_PORT[COM], &GPIO_InitStructure);

		USART_StructInit(&usart_def);
		usart_def.USART_BaudRate = baudrate;
		USART_Init(COM_USART[COM], &usart_def);

		/* Enable USART */
		USART_Cmd(COM_USART[COM], ENABLE);

		/* Enable the Receive interrupt: this interrupt is generated when the 
              receive data register is not empty */
		USART_ITConfig(COM_USART[COM], USART_IT_RXNE, ENABLE);
		
	}
}

uint16_t uart_getc(uint8_t COM)
{
	if(COM < COMn)
	{
		while (!USART_GetFlagStatus(COM_USART[COM], USART_FLAG_RXNE))
      		;

    	return USART_ReceiveData(COM_USART[COM]);
	}
	return -1;
}

void uart_putc(uint8_t COM, uint8_t c)
{
	if(COM < COMn)
	{
		while(!USART_GetFlagStatus(COM_USART[COM], USART_FLAG_TXE))
			;

	    USART_SendData(COM_USART[COM], c);
	}
}


void uart_putc_ex(uint8_t COM, uint8_t c)
{
	if(COM < COMn)
	{
		while(!USART_GetFlagStatus(COM_USART[COM], USART_FLAG_TXE))
			;

		/* \n ==> \r\n */
		if(c == '\n'){
			USART_SendData(COM_USART[COM], '\r');
			while(!USART_GetFlagStatus(COM_USART[COM], USART_FLAG_TXE))	;
		}
	    USART_SendData(COM_USART[COM], c);
	}
}



