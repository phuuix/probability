/* cc2530ctrl.c
 * MT of CC2530
 */
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "bsp.h"
#include "ipc.h"
#include "task.h"
#include "uart.h"
#include "uprintf.h"
#include "memory.h"
#include "stm32f2xx_usart.h"
#include "stm32f2xx_gpio.h"
#include "stm32f2xx_rcc.h"

#define CC2530_COM 1
#define INT_CC2530_COM (71+16) /* USART6 global interrupt */
#define CC2530_MT_BAUDRATE 38400

extern int zllctrl_post_zll_message(uint8_t *message, uint16_t length);
extern int zllctrl_init();

#define CDC_MSG_TYPE_TXREQ 2
#define CDC_MSG_TYPE_RXREQ 3

extern struct mailbox cdc_mbox;

/**
* @}
*/
void cc2530ctrl_mail_handle(uint32_t *mail)
{
	uint8_t type;
	uint8_t length, txlen = 0;
	uint8_t *message;

	type = (mail[0] & 0xFF000000)>>24;
	
	switch(type)
	{
		case CDC_MSG_TYPE_TXREQ:
			/* transfer a MT command to Soc */
			length = (mail[0] & 0x00FF0000)>>16;
			message = (uint8_t *)mail[1];
            kprintf("Send MT len=%d msg=0x%x: 0x%08x 0x%08x 0x%08x\n", 
                length, message, 
                *(uint32_t *)&message[0], *(uint32_t *)&message[4], *(uint32_t *)&message[8]);

			for(txlen=0; txlen<length; txlen++)
			{
				uart_putc(CC2530_COM, message[txlen]);
			}
            free(message);
			break;

		case CDC_MSG_TYPE_RXREQ:
			break;

		default:
			uprintf(UPRINT_WARNING, UPRINT_BLK_DEF, "unknown cdc command type: %d\n", type);
			break;
	}
}

static uint8_t cc2530_com_rxbuf[256];
static uint8_t cc2530_com_rxoffset;
static void cc2530_com_isr_func(int n)
{
    USART_TypeDef *pUsart = USART6;

    while((pUsart->SR & 0x20)) // RXNE interrupt on
    {
    	/* get one char from hw */
        cc2530_com_rxbuf[cc2530_com_rxoffset++] = USART_ReceiveData(pUsart);
        //kprintf("cc2530 isr: 0x%02x SR=%04x\n", cc2530_com_rxbuf[cc2530_com_rxoffset-1], pUsart->SR);
    	
    	if(cc2530_com_rxoffset >= 5)
    	{
    		/* 5: SOF + LEN + CMD0 + CMD1 + FCS */
    		if(cc2530_com_rxoffset >= cc2530_com_rxbuf[1]+5)
    		{
    			/* a full message is received */
    			zllctrl_post_zll_message(cc2530_com_rxbuf, cc2530_com_rxbuf[1]+4);
    			cc2530_com_rxoffset = 0;
    		}
    	}
    	else if(cc2530_com_rxbuf[0] != 0xFE)
    	{
    		/* 0xFE: (MT ROF), error with sync, reset Rx offset */
    		cc2530_com_rxoffset = 0;
    	}
    }
}

/* reset cc2530 by pull down GPIO D Pin13 */
void cc2530_reset()
{
	GPIO_InitTypeDef GPIO_InitStructure;

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_25MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL ;
	GPIO_Init(GPIOD, &GPIO_InitStructure);

    GPIO_ResetBits(GPIOD, GPIO_Pin_13);

    task_delay(10);

    GPIO_SetBits(GPIOD, GPIO_Pin_13);

    task_delay(10);
}

/**
  * adapt to OS
  * @}
  */ 
void cc2530ctrl_task(void *p)
{
	uint32_t mail[2];
	int ret;
	
	ret = mbox_initialize(&cdc_mbox, 2, 16, NULL);
	assert(ROK == ret);
	
    /* Reset CC2530 */
    cc2530_reset();

	/* Init USART6 */
	uart_init(CC2530_COM, CC2530_MT_BAUDRATE, USART_HardwareFlowControl_None);
	bsp_isr_attach(INT_CC2530_COM, cc2530_com_isr_func);
	bsp_irq_unmask(INT_CC2530_COM);
	
	zllctrl_init();
	
	for(;;)
	{
		ret = mbox_pend(&cdc_mbox, mail, 1);
		switch (ret)
		{
			case ROK:
				/* handle command: rx, tx ... */
				cc2530ctrl_mail_handle(mail);
				break;
			case RTIMEOUT:
				break;
			default:
				break;
		}
	}
}

void cc2530ctrl_init()
{
	task_t t;
	/* create tcdc task */
	t = task_create("tcc2530", cc2530ctrl_task, NULL, NULL, /* stack size */ 0x400, /* priority */ 4, 20, 0);
	assert(t > 0);
	task_resume_noschedule(t);
}

