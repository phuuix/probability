/* CH375 host mode functions */
#include <stdlib.h>
#include <string.h>

#include "task.h"
#include "ipc.h"
#include "ch375.h"
#include "libsamv.h"
#include "samv_agent.h"
#include "bsp.h"

#include "stm32f2xx_gpio.h"
#include "stm32f2xx_exti.h"
#include "stm32f2xx_rcc.h"
#include "stm32f2xx_syscfg.h"
#include "misc.h"

#define CH375_MDATASIZE 64


mbox_t ch375_mbox_int, ch375_mbox_msg;

extern void udelay(uint32_t us);
extern void ch375_isr_handler(int irq);
extern mbox_t samv_mbox;

ch375_t g_ch375;

//#define EMC_MODE
#if 0
#define CH375_DAT_PORT (*((volatile unsigned char *) 0x80000000)) 
#define CH375_CMD_PORT (*((volatile unsigned char *) 0x80000001)) 

/* Hardware connection with LPC2214
 * USBINTn:    P0.16/EINT0
 * CSn:           P1.0
 * OEn:           P1.1
 * A0:             P3.0
 * WEn:          P3.27
 * D[0..7]:      P2.[0..7]
 */

void ch375_wr_cmd_port( unsigned char cmd )
{
	CH375_CMD_PORT=cmd;
	udelay(2);
}

void ch375_wr_dat_port( unsigned char dat )
{
	CH375_DAT_PORT=dat;
	udelay(1);
}

unsigned char ch375_rd_dat_port() 
{
	unsigned char c;

	c = CH375_DAT_PORT;
	udelay(1);
	return c;
}
#else
/* Hardware connection with LPC2214
 * RSTn:         PE4
 * USBINTn:    PE3
 * CSn:           PC4
 * OEn:           PC2
 * A0:             PC3
 * WEn:          PE5
 * D[0..7]:      PA[0..7]
 */
static GPIO_InitTypeDef gpiodata;

#define CH375_INT   ( GPIO_Pin_3 )	//INT# PE3
#define CH375_INT_WIRE			( GPIOE->IDR & CH375_INT )

#define CH375_A0	( GPIO_Pin_3 ) 					   // PC3
#define CH375Data()  { GPIOC->BSRRH = CH375_A0; }      //A0 = 0
#define CH375Cmd()   { GPIOC->BSRRL = CH375_A0; }      //A0 = 1

#define CH375_CS	( GPIO_Pin_4 )                        // PC4
#define CH375Select()  { GPIOC->BSRRH = CH375_CS; __NOP(); __NOP(); }    //CS# = 0
#define CH375UnSelect()  { GPIOC->BSRRL = CH375_CS; }  //CS# = 1

#define CH375_WR	( GPIO_Pin_5 )                     // PE5
#define CH375Write()  { GPIOE->BSRRH = CH375_WR; }     //WR# = 0
#define CH375UnWrite()  { GPIOE->BSRRL = CH375_WR; }   //WR# = 1

#define CH375_RD	( GPIO_Pin_2 )                     // PC2
#define CH375Read()  { GPIOC->BSRRH = CH375_RD; }      //RD# = 0
#define CH375UnRead()  { GPIOC->BSRRH = CH375_RD; }    //RD# = 1

#define CH375_RST   ( GPIO_Pin_4 )                     // PE4

#define CH375_DATA  ( 0xFF )                           // PA[0..7]
#define CH375DataIn() { \
	if(gpiodata.GPIO_Mode != GPIO_Mode_IN){ \
		gpiodata.GPIO_Mode = GPIO_Mode_IN; \
		GPIO_Init(GPIOA, &gpiodata); \
		__NOP(); __NOP(); \
	}}
#define CH375DataOut() { \
	if(gpiodata.GPIO_Mode != GPIO_Mode_OUT){ \
		gpiodata.GPIO_Mode = GPIO_Mode_OUT; \
		GPIO_Init(GPIOA, &gpiodata); \
		__NOP(); __NOP(); \
	}}

void stm32_ch375_init(void)   
{
	GPIO_InitTypeDef GPIO_InitStructure;
	EXTI_InitTypeDef   EXTI_InitStructure;
	NVIC_InitTypeDef   NVIC_InitStructure;
	
	RCC_AHB1PeriphClockCmd( RCC_AHB1Periph_GPIOA|RCC_AHB1Periph_GPIOC|RCC_AHB1Periph_GPIOE, ENABLE);  
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);
	
	/* D[0..7] Pins */
	gpiodata.GPIO_Pin = CH375_DATA;

	gpiodata.GPIO_Speed = GPIO_Speed_25MHz;
	gpiodata.GPIO_Mode = GPIO_Mode_IN;
	gpiodata.GPIO_OType = GPIO_OType_PP;
	gpiodata.GPIO_PuPd = GPIO_PuPd_NOPULL ;
	GPIO_Init(GPIOA, &gpiodata);

	/* CSn Pin, OE and A0 Pins */
	GPIO_InitStructure.GPIO_Pin = CH375_RD | CH375_A0 | CH375_CS;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_25MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL ;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
	
	/* WE and RSTn Pins */
	GPIO_InitStructure.GPIO_Pin = CH375_RST | CH375_WR;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_25MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL ;
	GPIO_Init(GPIOE, &GPIO_InitStructure);
	
	/* USBINTn Pin : Configure PE3 pin as input floating */
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_InitStructure.GPIO_Pin = CH375_INT;
	GPIO_Init(GPIOE, &GPIO_InitStructure);

	/* Connect EXTI Line3 to PE3 pin */
	SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOE, EXTI_PinSource3);

	/* Configure EXTI Line3 */
	EXTI_InitStructure.EXTI_Line = EXTI_Line3;
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStructure);

	/* Enable and set EXTI Line0 Interrupt to the lowest priority */
	bsp_isr_attach(EXTI3_IRQn+16, ch375_isr_handler);
	
	NVIC_InitStructure.NVIC_IRQChannel = EXTI3_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x0F;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x0F;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
	// disable ch375 interrupt until set usb mode success
	//irq_mask(EXTI0_IRQn+16);
	
	/* set CS to 1 and RSTn to 0 */
	GPIO_ResetBits(GPIOE, CH375_RST);
	CH375UnSelect();
	CH375UnRead();
	CH375UnWrite();
	
	/* delay 50ms */
	KPRINTF("delay 50ms for ch375 reset\n");
	task_delay(50/(1000/TICKS_PER_SECOND));
}

void SetCH375WriteCmd()
{
	CH375Select();
	CH375Cmd();
	//CH375UnRead();
	CH375Write();
}

void SetCH375WriteData()
{
	CH375Select();
	CH375Data();
	//CH375UnRead();
	CH375Write();
}

void UnSetCH375Write()
{
	CH375UnWrite();
	CH375UnSelect();
}

void SetCH375ReadData()
{
	CH375Select();
	CH375Data();
	//CH375UnWrite();
	CH375Read();
}

void UnSetCH375Read()
{
	CH375UnRead();
	CH375UnSelect();
}

void ch375_wr_cmd_port( unsigned char mCmd )
{
	//udelay( 2 );
	CH375DataOut();
	*(uint8_t *)&(GPIOA->ODR) = mCmd;
	SetCH375WriteCmd();
	__NOP(); __NOP();
	UnSetCH375Write();
	udelay( 4 );
	
}

void ch375_wr_dat_port( unsigned char mData )
{
	CH375DataOut();
	*(uint8_t *)&(GPIOA->ODR) = mData;
	SetCH375WriteData();
	__NOP(); __NOP();
	UnSetCH375Write();
	udelay( 2 );
}

unsigned char ch375_rd_dat_port( void )
{
	unsigned char	mData;
	//udelay( 2 );
	CH375DataIn();
	SetCH375ReadData();
	mData = GPIOA->IDR;
	__NOP(); __NOP();
	UnSetCH375Read();
	udelay( 2 );
	return( mData );
}

#endif

/* host wait for interrupts */
unsigned char ch375_wait_interrupt(unsigned short ms) 
{
	uint32_t mail;
	
	/* init as USB_INT_TIMEOUT event */
	mail = USB_INT_TIMEOUT;
	mbox_pend(&ch375_mbox_int, &mail, ms/10);

	return mail;
}

unsigned char ch375_check_exist()
{
	unsigned char v;

	v = ch375_rd_dat_port();
	
	ch375_wr_cmd_port( CMD_CHECK_EXIST);
	ch375_wr_dat_port( 0x57 );

	v = ch375_rd_dat_port();
	if(v != 0xA8)
		KPRINTF("ch375: chip doesn't exist, v=0x%x\n", v);
	return (v == 0xA8);
}

unsigned char ch375_set_usb_mode( unsigned char mode ) 
{
	uint16 i, ret;
	uint32_t f;

	f = bsp_fsave();
	ch375_wr_cmd_port( CMD_SET_USB_MODE );
	ch375_wr_dat_port( mode );
	for( i=0; i<100; i++ ) {  /* usually finish in 20uS */
		ret = ch375_rd_dat_port();
		if ( ret==CMD_RET_SUCCESS )
		{
			bsp_frestore(f);
			return( TRUE );
		}
	}
	bsp_frestore(f);
	KPRINTF("ch375: Warning: can't set usb mode to %d, ret=0x%x\n", mode, ret);
	return( FALSE );
}

/* read data from ch375 into buffer */
unsigned char ch375_rd_usb_data( unsigned char *buf ) 
{
	unsigned char i, len;
	ch375_wr_cmd_port( CMD_RD_USB_DATA );
	len=ch375_rd_dat_port();
	for ( i=0; i<len && i<64; i++ ) 
		*buf++=ch375_rd_dat_port();
	return( len );
}

/* write data into ch375: len must <=64 */
void ch375_wr_usb_data( unsigned char len, unsigned char *buf ) 
{
	ch375_wr_cmd_port( CMD_WR_USB_DATA7 );
	ch375_wr_dat_port( len );
	while( len-- ) 
		ch375_wr_dat_port( *buf++ );
}


unsigned char ch375_issue_token( unsigned char endp_and_pid ) 
{
#if 0
	if((endp_and_pid & 0xF) == DEF_USB_PID_IN)
		KPRINTF("ch375: issue a IN token: endp=%d\n", (endp_and_pid >> 4));
	else
		KPRINTF("ch375: issue a OUT token: endp=%d\n", (endp_and_pid >> 4));
#endif
	
	/* issue USB transaction */
	ch375_wr_cmd_port( CMD_ISSUE_TOKEN );
	/* endp: 4 bits; token pid: 4 bits */
	ch375_wr_dat_port( endp_and_pid );

	return( ch375_wait_interrupt(3000) ); //at most wait 3s
}

void ch375_toggle_recv(ch375_t *ch375, uint8_t ep) 
{
	/* host receive sync indicator: 0=DATA0,1=DATA1 */
	ch375_wr_cmd_port( CMD_SET_ENDP6 );
	ch375_wr_dat_port(ch375->endpoint[ep].tog_data);
	ch375->endpoint[ep].tog_data ^= 0x40;
	udelay(4);
}


void ch375_toggle_send(ch375_t *ch375, uint8_t ep) 
{  
	/* host send sync indicator: 0=DATA0,1=DATA1 */
	ch375_wr_cmd_port( CMD_SET_ENDP7 );
	ch375_wr_dat_port( ch375->endpoint[ep].tog_data );
	ch375->endpoint[ep].tog_data ^= 0x40;
	udelay(4);
}

/* clear endpoint's stall status and sync to data0 */
unsigned char ch375_clr_stall(ch375_t *ch375, unsigned char ep ) 
{
	unsigned char endp_addr;

	endp_addr = ch375->endpoint[ep].address | ch375->endpoint[ep].direction;

	KPRINTF("CH375 clear stall: endp_addr=0x%x\n", endp_addr);
	
	ch375_wr_cmd_port( CMD_CLR_STALL );
	ch375->endpoint[ep].tog_data = CH375_TOG_DATA0;
	ch375_wr_dat_port( endp_addr );
	return( ch375_wait_interrupt(500) );
}


#define	USB_INT_RET_NAK		0x2A		/* 00101010B,·µ»ØNAK */
/* len: >0; =0: end sending
 * return:
 *    >0 success
 *    =0 recover success
 *    <0 error
 */
int ch375_host_send(ch375_t *ch375, unsigned short len, unsigned char *buf ) 
{
	unsigned short l, ret;
	uint8_t endp_out_size;
	uint8_t endp_out_addr;
	uint8_t ep = 1;

	endp_out_size =  ch375->endpoint[ep].max_packet_size;
	endp_out_addr = ch375->endpoint[ep].address;
	
	if(ch375->endpoint[3].address > 0)
	{
		l = len>SAMV_HEADER_SIZE?SAMV_HEADER_SIZE:len;
		ch375_wr_usb_data(l, buf);
		ch375_toggle_send(ch375, ep);
		ret = ch375_issue_token( ( endp_out_addr << 4 ) | DEF_USB_PID_OUT );
		if(ret & 0x20) // failure case
		{
			KPRINTF("ch375: USB send error: 0x%x, try to recovery...\n", ret);
			ret = ch375_clr_stall(ch375, ep);
			return ret==USB_INT_SUCCESS?0:-1;
		}
		else if(ret==USB_INT_TIMEOUT){
			KPRINTF("ch375: USB send timeout, endp_out_addr=%d.\n", endp_out_addr);
			return -1;
		}

		len -= l;
		buf += l;

		/* switch to next endpoint */
		ep = 3;
		endp_out_size = ch375->endpoint[ep].max_packet_size;
		endp_out_addr = ch375->endpoint[ep].address;
	}
	
	do
	{
		l = len>endp_out_size?endp_out_size:len;
		
		/* write data to chip */
		ch375_wr_usb_data( l, buf );
		/* sync data */
		ch375_toggle_send( ch375, ep);
		/* request chip to output */
		ret = ch375_issue_token( ( endp_out_addr << 4 ) | DEF_USB_PID_OUT );
		if(ret & 0x20) // failure case
		{
			KPRINTF("ch375: USB send error: 0x%x, try to recovery...\n", ret);
			ret = ch375_clr_stall(ch375, ep);
			return ret==USB_INT_SUCCESS?0:-1;
		}
		
		if ( ret==USB_INT_SUCCESS ) {
			//send success
			len-=l;
			buf+=l;
		}
		else if(ret==USB_INT_TIMEOUT){
			KPRINTF("ch375: USB send timeout.\n");
			return -1;
		}
		else{
			/* regard others interrupts as unexpected and throw to upper */
			KPRINTF("USB send unexpected interrupt: 0x%x\n", ret);
			return -1;
		}
	}while( len > 0 );

	return 1;
}

#if 0
/* return the length of rx bytes:
 *  length > 0: valid data and need continue receive
 *  length = 0: no more data or recover from clear stall
 *  length < 0: error
 */
int ch375_host_recv(ch375_t *ch375, unsigned char *buf )
{
	unsigned char ret;
	int len;
	
	ch375_toggle_recv(ch375);
	
	ret = ch375_issue_token( ( ch375->endpoint[0].address << 4 ) | DEF_USB_PID_IN );
	if(ret & 0x20)
	{
		KPRINTF("ch375: USB recv error: 0x%x, try to recovery...\n", ret);
		ret = ch375_clr_stall(ch375, ch375->endpoint[0].address, 0);
		return (ret==USB_INT_SUCCESS)?0:-1;
	}

	if(ret == USB_INT_TIMEOUT)
	{
		KPRINTF("ch375: warning: USB issue tocken timeout\n");

		ch375_toggle_recv(ch375);
	
		ret = ch375_issue_token( ( ch375->endpoint[0].address << 4 ) | DEF_USB_PID_IN );
		KPRINTF("ret=0x%x\n", ret);
		return 0;
	}
	else if ( ret != USB_INT_SUCCESS )
	{
		/* throw to upper */
		KPRINTF("ch375: warning: recv unexpectd interrupt: 0x%x\n", ret);
		return -1;
	}
	
	len = ch375_rd_usb_data( buf );

	return len;
}
#endif
/* return the length of rx bytes:
 *  length > 0: valid data and need continue receive
 *  length = 0: no more data or recover from clear stall
 *  length < 0: error
 */
int ch375_host_recv(ch375_t *ch375, unsigned char *buf, uint32_t bufsize)
{
	unsigned char ret;
	int len = 0;
	uint8_t endp_in_size;
	uint8_t endp_in_addr;
	uint8_t ep = 0;

	endp_in_size =  ch375->endpoint[ep].max_packet_size;
	endp_in_addr = ch375->endpoint[ep].address;
	
	if(ch375->endpoint[2].address > 0)
	{
		/* recevie SAMV header */
		ch375_toggle_recv(ch375, ep);
		ret = ch375_issue_token( ( endp_in_addr << 4 ) | DEF_USB_PID_IN );
		if(ret & 0x20)
		{
			KPRINTF("ch375: USB recv error: 0x%x, try to recovery...\n", ret);
			ret = ch375_clr_stall(ch375, ep);
			return (ret==USB_INT_SUCCESS)?0:-1;
		}
		else if(ret == USB_INT_TIMEOUT)
		{
			KPRINTF("ch375: warning: USB issue tocken timeout, endp_in_addr=%d\n", endp_in_addr);
			return -1;
		}

		ret = ch375_rd_usb_data( buf );
		assert(bufsize > ret);
		buf += ret;
		len += ret;
		bufsize -= ret;

		/* switch to next endpoint 2 */
		ep = 2;
		endp_in_size = ch375->endpoint[ep].max_packet_size;
		endp_in_addr = ch375->endpoint[ep].address;
	}

	do{
		ch375_toggle_recv(ch375, ep);
		
		ret = ch375_issue_token( ( endp_in_addr << 4 ) | DEF_USB_PID_IN );
		if(ret & 0x20)
		{
			KPRINTF("ch375: USB recv error: 0x%x, try to recovery...\n", ret);
			ret = ch375_clr_stall(ch375, ep);
			return (ret==USB_INT_SUCCESS)?0:-1;
		}
		else if(ret == USB_INT_TIMEOUT)
		{
			KPRINTF("ch375: warning: USB issue tocken timeout\n");
			return -1;
		}
		
		ret = ch375_rd_usb_data( buf );
		assert(bufsize > ret);
		buf += ret;
		len += ret;
		bufsize -= ret;
	}while(ret == endp_in_size);

	return len;
}


/* get descriptor: 1 device; 2 configure */
unsigned char ch375_get_descr( unsigned char type ) 
{
	KPRINTF("CH375 get descriptor: type=%d\n", type);
	
	ch375_wr_cmd_port( CMD_GET_DESCR );
	ch375_wr_dat_port( type );
	return( ch375_wait_interrupt(200) );
}

/* set address of USB device */
unsigned char ch375_set_addr( unsigned char addr )
{
	unsigned char status;

	KPRINTF("CH375 set addr: addr=%d\n", addr);

	/* set device side address */
	ch375_wr_cmd_port( CMD_SET_ADDRESS );
	/* adress, from 1~127 */
	ch375_wr_dat_port( addr );
	status=ch375_wait_interrupt(100);
	if ( status==USB_INT_SUCCESS ) {
		/* set host side address */
		ch375_wr_cmd_port( CMD_SET_USB_ADDR );
		ch375_wr_dat_port( addr );
	}
	task_delay( 1 );
	return( status );
}

/* configure USB device */
unsigned char ch375_set_config(ch375_t *ch375,  unsigned char cfg ) 
{
	uint32_t i;
	
	KPRINTF("CH375 set config: cfg=%d\n", cfg);
	
	/* sync as data 0 */
	for(i=0; i<CH375_MAX_ENDPOINTS; i++)
		ch375->endpoint[i].tog_data = CH375_TOG_DATA0;

	ch375_wr_cmd_port( CMD_SET_CONFIG );
	/* cfg comes from device's configure discriptor */
	ch375_wr_dat_port( cfg );
	return( ch375_wait_interrupt(100) );
}

void ch375_set_retry(unsigned char value)
{
	ch375_wr_cmd_port( CMD_SET_RETRY );
	ch375_wr_dat_port( 0x25 );
	ch375_wr_dat_port( value );
}

void ch375_set_speed(unsigned char speed)
{
	ch375_wr_cmd_port(CMD_SET_USB_SPEED);
	ch375_wr_dat_port(speed);
}


void ch375_isr_handler(int irq)
{
	uint32_t intr;

//	if(EXTI_GetITStatus(EXTI_Line3) != RESET)
	{
		/* get interrupt status */
		ch375_wr_cmd_port( CMD_GET_STATUS );
		intr = ch375_rd_dat_port();

		//KPRINTF("CH375 interrupt: 0x%x\n", intr);
		
		/* post a host interrupt to tch375 */
		if(intr > 0x0F)
			mbox_post(&ch375_mbox_int, (uint32_t *)&intr);

		/* Clear the EXTI line 0 pending bit */
    	EXTI_ClearITPendingBit(EXTI_Line3);
	}
}

/* set USB mode as following sequence:
 *     7==>6==>7==>6 
 */
void ch375_reset(ch375_t *ch375)
{
	unsigned char mode;
	uint8_t i;

	/* reset endpoint info */
	for(i=0; i<CH375_MAX_ENDPOINTS; i++)
	{
		ch375->endpoint[i].address = 0;
		ch375->endpoint[i].tog_data = CH375_TOG_DATA0;
		ch375->endpoint[i].max_packet_size = 0;
	}
	ch375->endpoint[0].direction = ch375->endpoint[2].direction = CH375_ENDP_DIRECT_IN;
	ch375->endpoint[1].direction = ch375->endpoint[3].direction = CH375_ENDP_DIRECT_OUT;
#if 0
	/* set usb mode 7: USB host, reset device */
	mode = 7;
	if(ch375_set_usb_mode(mode))
	{
		KPRINTF("ch375: set usb mode to %d\n", mode);
		//ch375->state = CH375_STATE_ERROR;
		//return;
	}
	//irq_unmask(EXTI0_IRQn+16);
	task_delay(20/(1000/TICKS_PER_SECOND));
#endif	
	/* set usb mode 6: USB host, SOF, detect device */
	mode = 6;
	if(ch375_set_usb_mode(mode))
	{
		KPRINTF("ch375: set usb mode to %d\n", mode);
		//ch375->state = CH375_STATE_ERROR;
		//return;
	}

	/* wait for device connection interrupt */
	while(ch375_wait_interrupt(100) != USB_INT_CONNECT);

	KPRINTF("ch375: One device connected\n");
	
	/* set usb mode 7: USB host, reset device */
	mode = 7;
	if(ch375_set_usb_mode(mode))
	{
		KPRINTF("ch375: set usb mode to %d\n", mode);
		//ch375->state = CH375_STATE_ERROR;
		//return;
	}
	
	task_delay(20/(1000/TICKS_PER_SECOND));


	/* set usb mode 6: USB host, SOF, detect device */
	mode = 6;
	if(ch375_set_usb_mode(mode))
	{
		KPRINTF("ch375: set usb mode to %d\n", mode);
		//ch375->state = CH375_STATE_ERROR;
		//return;
	}

	/* wait for device connection interrupt */
	while(ch375_wait_interrupt(100) != USB_INT_CONNECT);
	
	KPRINTF("ch375: Device connects again\n");

	/* some device need to delay upto ~200 ms */
	task_delay(400/(1000/TICKS_PER_SECOND));

	/* switch to not enum state */
	ch375->state = CH375_STATE_NOTENUM;
}

extern int samv_agent_get_samvid_cb();
void ch375_enum(ch375_t *ch375)
{
	unsigned char buffer[64];
	PUSB_DEV_DESCR p_dev_descr = ((PUSB_DEV_DESCR)buffer);
 	PUSB_CFG_DESCR_LONG p_cfg_descr = ((PUSB_CFG_DESCR_LONG)buffer);
	unsigned char status, len, c, i;

	/* get device descriptor */
	status=ch375_get_descr(1);
	if ( status!=USB_INT_SUCCESS ) 
	{
		KPRINTF("ch375: warning: can't get device descriptor: ret=0x%x\n", status);
		ch375->state = CH375_STATE_NOTCNCT;
		return;
	}

	/* read descriptor data
	 * Class 0: Device class is unspecified, interface descriptors are used to determine needed drivers
	 * Type 1: USB_DEVICE_DESCRIPTOR_TYPE
	 */
	len=ch375_rd_usb_data( buffer );
	if ( len<18 || p_dev_descr->bDescriptorType!=USB_DEVICE_DESCRIPTOR_TYPE || p_dev_descr->bDeviceClass!=0){
		KPRINTF("ch375: Invalid DEV_DESCR data: len=%d type=%d class=%d\n", 
				len, p_dev_descr->bDescriptorType, p_dev_descr->bDeviceClass);
		ch375->state = CH375_STATE_NOTCNCT;
		return;
	}

	KPRINTF("ch375: USB DEV_DESCR: bcdUSB=0x%x bDeviceClass=0x%x bDeviceSubClass=0x%x bDeviceProtocol=0x%x\n", 
			p_dev_descr->bcdUSB, p_dev_descr->bDeviceClass, 
			p_dev_descr->bDeviceSubClass, p_dev_descr->bDeviceProtocol);
	KPRINTF("               bMaxPacketSize0=%d idVendor=0x%x idProduct=0x%x bcdDevice=0x%x\n",
			p_dev_descr->bMaxPacketSize0, p_dev_descr->idVendor, p_dev_descr->idProduct,
			p_dev_descr->bcdDevice);
	KPRINTF("               iManufacturer=%d iProduct=%d iSerialNumber=%d bNumConfigurations=%d\n",
			p_dev_descr->iManufacturer, p_dev_descr->iProduct, p_dev_descr->iSerialNumber,
			p_dev_descr->bNumConfigurations);
	dump_buffer(buffer, len);

	ch375->usb_version = p_dev_descr->bcdUSB;
	
	/* set device's USB address */
	status=ch375_set_addr(5);
	if ( status!=USB_INT_SUCCESS ){
		KPRINTF("ch375: warning: Can't set device address: ret=0x%x\n", status);
		ch375->state = CH375_STATE_NOTCNCT;
		return;
	}
	KPRINTF("ch375: set USB device address to 5\n");

	/* get configure descriptor */
	status=ch375_get_descr(2);
	if ( status!=USB_INT_SUCCESS ) {
		KPRINTF("ch375: warning: Can't get configuration descriptor: ret=0x%x\n", status);
		ch375->state = CH375_STATE_NOTCNCT;
		return;
	}

	/* read descriptor data */
	len=ch375_rd_usb_data( buffer );

	KPRINTF("ch375: USB CFG_DESCR: bLength=%d bDescriptorType=%d wTotalLength=%d bNumInterfaces=%d\n", 
			p_cfg_descr->cfg_descr.bLength, p_cfg_descr->cfg_descr.bDescriptorType, 
			p_cfg_descr->cfg_descr.wTotalLength, p_cfg_descr->cfg_descr.bNumInterfaces);
	KPRINTF("               bConfigurationValue=0x%x iConfiguration=%d bmAttributes=0x%x MaxPower=0x%x\n",
			p_cfg_descr->cfg_descr.bConfigurationValue, p_cfg_descr->cfg_descr.iConfiguration,
			p_cfg_descr->cfg_descr.bmAttributes, p_cfg_descr->cfg_descr.MaxPower);
	KPRINTF("ch375: USB ITF_DESCR : bLength=%d bDescriptorType=%d bInterfaceNumber=%d bNumEndpoints=%d\n",
			p_cfg_descr->itf_descr.bLength, p_cfg_descr->itf_descr.bDescriptorType, p_cfg_descr->itf_descr.bInterfaceNumber,
			p_cfg_descr->itf_descr.bNumEndpoints);
	KPRINTF("               bInterfaceClass=0x%x bInterfaceSubClass=0x%x bInterfaceProtocol=0x%x iInterface=%d\n",
			p_cfg_descr->itf_descr.bInterfaceClass, p_cfg_descr->itf_descr.bInterfaceSubClass,
			p_cfg_descr->itf_descr.bInterfaceProtocol, p_cfg_descr->itf_descr.iInterface);
	for(i=0; i<p_cfg_descr->itf_descr.bNumEndpoints && i<CH375_MAX_ENDPOINTS; i++)
	{
		KPRINTF("ch375: USB ENDP_DESCR %d: bDescriptorType=%d bEndpointAddress=0x%x bmAttributes=0x%x wMaxPacketSize=%d bInterval=%d\n",
			i, p_cfg_descr->endp_descr[i].bDescriptorType, p_cfg_descr->endp_descr[i].bEndpointAddress,
			p_cfg_descr->endp_descr[i].bmAttributes, p_cfg_descr->endp_descr[i].wMaxPacketSize, p_cfg_descr->endp_descr[i].bInterval);
	}
	dump_buffer(buffer, len);

	for(i=0; i<CH375_MAX_ENDPOINTS; i++)
		ch375->endpoint[i].address = 0;
	for(i=0; i<p_cfg_descr->itf_descr.bNumEndpoints && i<CH375_MAX_ENDPOINTS; i++){
		uint8_t point;
		c=p_cfg_descr->endp_descr[i].bEndpointAddress;
		if((c&0x7F) == 0)
			continue;   // bypass control endpoint
		if(p_cfg_descr->endp_descr[i].bmAttributes != 0x02)
			continue;	// need bulk transfer type
		
		if(c&0x80)      // 0&2 for in endpoint and 1&3 for out endpoint
			point = ch375->endpoint[0].address?2:0;
		else
			point = ch375->endpoint[1].address?3:1;
		ch375->endpoint[point].max_packet_size = p_cfg_descr->endp_descr[i].wMaxPacketSize;
		if(ch375->endpoint[point].max_packet_size>64 || ch375->endpoint[point].max_packet_size==0)
			ch375->endpoint[point].max_packet_size=64;
		ch375->endpoint[point].address = c&0x0f;
		if(p_cfg_descr->endp_descr[i].bInterval != 0)
			ch375->endpoint[point].interval = (1<<(p_cfg_descr->endp_descr[i].bInterval-1))*125/1000; //in ms
		else
			ch375->endpoint[point].interval = 0;
	}

	if(ch375->endpoint[0].address ==0 || ch375->endpoint[1].address ==0)
	{
		KPRINTF("ch375: Invalid endpoint address: in=%d, out=%d\n", 
			ch375->endpoint[0].address, ch375->endpoint[1].address);
		ch375->state = CH375_STATE_NOTCNCT;
		return;
	}

	/* set configure */
	status=ch375_set_config(ch375, p_cfg_descr->cfg_descr.bConfigurationValue );
	if ( status==USB_INT_SUCCESS ) {
		ch375_set_retry(0xC3); // retry 200ms~2s
	}
	else{
		KPRINTF("ch375: warning: Can't set config: ret=0x%x\n", status);
		ch375->state = CH375_STATE_NOTCNCT;
		return;
	}

#if 0
	if(ch375->usb_version == 0x100){
		/* set low speed */
		KPRINTF("set USB speed to 1.5M\n");
		ch375_set_speed(2);
	}
#endif

	KPRINTF("ch375: USB ENUM success.\n");

	/* some device need to delay upto ~200 ms */
	task_delay(200/(1000/TICKS_PER_SECOND));
	if(samv_agent_get_samvid_cb() == 0)
		ch375->state = CH375_STATE_WORKING;
	else
		ch375->state = CH375_STATE_NOTCNCT;
}

void ch375_working(ch375_t *ch375)
{
	ch375_mail_t mail;
	int16_t len = 0;
	samv_mail_t samv_mail;
	uint8_t *bufptr;
	
	/* wait for message from samv */
	do{
		mail.mh.length = 0;
		mbox_pend(&ch375_mbox_msg, (uint32_t *)&mail, 5);

		if(mail.mh.length == 0)
		{
			/* check device disconnect */
			mail.interrupt = 0;
			mbox_pend(&ch375_mbox_int, (uint32_t *)&mail.interrupt, 0);
			if(mail.interrupt== USB_INT_DISCONNECT)
			{
				KPRINTF("ch375: USB device disconnected\n");
				break;
			}
			else if(mail.interrupt != 0)
				KPRINTF("ch375: Ignore USB interrupt 0x%x\n", mail.interrupt);
			
			continue;
		}

#ifdef _DEBUG
		KPRINTF("ch375: rx host message: length=%d\n", mail.mh.length);
		dump_buffer(mail.mh.bufptr, mail.mh.length);
#endif
		
		/* send  message via USB */
		len = ch375_host_send(ch375, mail.mh.length, mail.mh.bufptr);
		free(mail.mh.bufptr);	//free buffer
		if(len < 0)
		{
			KPRINTF("ch375: send usb error.\n");
			break;
		}

		task_delay(1);

		/* receive data */
		bufptr = malloc(1600);
		if(bufptr){
			len = ch375_host_recv(ch375, bufptr, 1600);
			if(len > 0)
			{
				samv_mail.bufptr = bufptr;
				samv_mail.event = SAMVA_EVENT_TARGETMSG;
				samv_mail.length = len;
				samv_mail.port = 0;
#ifdef _DEBUG
				KPRINTF("ch375: rx target message: length=%d\n", len);
				dump_buffer(bufptr, 11);
#endif
				if(mbox_post(&samv_mbox, (uint32_t *)&samv_mail) != 0)
				{
					KPRINTF("ch375: warning: samv mbox full!\n");
					free(bufptr);
				}
			}
			else{
				free(bufptr);
			}
		}
	}while(len >= 0);

	/* require reset */
	task_delay(1000/(1000/TICKS_PER_SECOND));
	ch375->state = CH375_STATE_NOTCNCT;
}

void ch375_task(void *param)
{
	ch375_t *ch375 = (ch375_t *)param;

	mbox_initialize(&ch375_mbox_int, 1 /* 1 long word */, 16, NULL);
	mbox_initialize(&ch375_mbox_msg, 2 /* 2 long word */, 16, NULL);

	stm32_ch375_init();
	
	KPRINTF("ch375 task started.\n");
	if(!ch375_check_exist())
	{
		KPRINTF("ch375 chip doesn't exist\n");
		goto ch375_end;
	}

	while(ch375->state != CH375_STATE_ERROR)
	{
		switch(ch375->state)
		{
			case CH375_STATE_NOTCNCT:
				/* after reset, state is switched to NOTENUM */
				ch375_reset(ch375);
				break;
			case CH375_STATE_NOTENUM:
				ch375_enum(ch375);
				break;
			case CH375_STATE_WORKING:
				ch375_working(ch375);
				break;
			default:
				KPRINTF("ch375: error! unexpected state %d\n", ch375->state);
				break;
		}
	}

ch375_end:
	bsp_irq_mask(EXTI3_IRQn+16);
	bsp_isr_detach(EXTI3_IRQn+16);

	mbox_destroy(&ch375_mbox_int);
	mbox_destroy(&ch375_mbox_msg);

	KPRINTF("ch375 task ended.\n");
}

void ch375_set_usbid(uint16_t vendor_id, uint16_t product_id)
{
	uint8_t *c;
	
	KPRINTF("CH375 set vendorId=0x%x productId=0x%x\n", vendor_id, product_id);

	/* set host side address */
	ch375_wr_cmd_port( CMD_SET_USB_ID );
	c = (uint8_t *)&vendor_id;
	ch375_wr_dat_port( c[0] );
	ch375_wr_dat_port( c[1] );
	c = (uint8_t *)&product_id;
	ch375_wr_dat_port( c[0] );
	ch375_wr_dat_port( c[1] );
}

uint8_t ch375_get_state()
{
	return g_ch375.state;
}

void ch375_init()
{
	task_t tt;

	ch375_t *ch375 = &g_ch375;

	ch375->state = CH375_STATE_NOTCNCT;
	
	/* create ch375 task */
	tt = task_create("tch375", ch375_task, ch375, NULL, 0x400 /* stack size */, 3 /* priority */, 10, 0);
	//tt = task_create("tch372", ch372_task, &g_ch375, NULL, 0x300 /* stack size */, 4 /* priority */, 0, 0);
	task_resume_noschedule(tt);
}

