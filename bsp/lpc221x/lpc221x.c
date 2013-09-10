#include <string.h>
#include <config.h>
#include "lpc2xxx.h"


/*
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*/
typedef            void     (*CPU_FNCT_VOID)(void);             /* See Note #2a.                                        */
typedef            void     (*CPU_FNCT_PTR )(void *);           /* See Note #2b.                                        */

/*
*********************************************************************************************************
*                                       LOCAL GLOBAL VARIABLES
*********************************************************************************************************
*/
        uint32_t  VIC_SpuriousInt;
/*
*********************************************************************************************************
*                                      LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*/
static  void  lpc221x_pll_init         (void);
static  void  lpc221x_mam_init         (void);
static  void  lpc221x_timer_init     (void);
static  void  lpc221x_vic_init         (void);

/*
*********************************************************************************************************
*********************************************************************************************************
**                                         GLOBAL FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/
void lpc221x_delay(void)
{
	// delay ~1ms on 60MHz CPU
	int n = 0x7FFF;

	while(n>0)
		n--;
}


/*
*********************************************************************************************************
*                                             lpc221x_init()
*
* Description : Initialize the Board Support Package (BSP).
*
* Argument(s) : none.
*
* Return(s)   : none.
*
* Note(s)     : (1) This function SHOULD be called before any other BSP function is called.
*********************************************************************************************************
*/

void  lpc221x_init (void)
{
    MEMMAP  = 1;
	
    /* set System Control and Status flags register: GPIO mode */
    SCS = 0;
	
    lpc221x_pll_init();                                             /* Initialize PLL0 and the VPB Divider Register             */
    lpc221x_mam_init();                                             /* Initialize the Memory Acceleration Module                */
    lpc221x_vic_init();                                             /* Initialize the Vectored Interrupt Controller             */
	
    lpc221x_timer_init();                                           /* Initialize the uC/OS-II tick interrupt                   */
}


/*
*********************************************************************************************************
*                                          OS_CPU_ExceptHndlr()
*
* Description : Handle any exceptions.
*
* Argument(s) : except_id     ARM exception type:
*
*                                  OS_CPU_ARM_EXCEPT_RESET             0x00
*                                  OS_CPU_ARM_EXCEPT_UNDEF_INSTR       0x01
*                                  OS_CPU_ARM_EXCEPT_SWI               0x02
*                                  OS_CPU_ARM_EXCEPT_PREFETCH_ABORT    0x03
*                                  OS_CPU_ARM_EXCEPT_DATA_ABORT        0x04
*                                  OS_CPU_ARM_EXCEPT_ADDR_ABORT        0x05
*                                  OS_CPU_ARM_EXCEPT_IRQ               0x06
*                                  OS_CPU_ARM_EXCEPT_FIQ               0x07
*
* Return(s)   : none.
*
* Caller(s)   : OS_CPU_ARM_EXCEPT_HANDLER(), which is declared in os_cpu_a.s.
*********************************************************************************************************
*/
#if 0
void  OS_CPU_ExceptHndlr (uint32_t  except_id)
{
    CPU_FNCT_VOID  pfnct;
    
    if (except_id == OS_CPU_ARM_EXCEPT_IRQ) {

        pfnct = (CPU_FNCT_VOID)VICVectAddr;                     /* Read the interrupt vector from the VIC                   */
        while (pfnct != (CPU_FNCT_VOID)0) {                     /* Make sure we don't have a NULL pointer                   */
          (*pfnct)();                                           /* Execute the ISR for the interrupting device              */
            VICVectAddr = 1;                                    /* Acknowlege the VIC interrupt                             */
            pfnct = (CPU_FNCT_VOID)VICVectAddr;                 /* Read the interrupt vector from the VIC                   */
        }

    } else {
                                                                /* Infinite loop on other exceptions.                       */
                                                                /* Should be replaced by other behavior (reboot, etc.)      */
        while (DEF_TRUE) {
            ;
        }
    }
}
#endif

/*
*********************************************************************************************************
*                                           lpc221x_int_disable()
*
* Description : Disable ALL interrupts.
*
* Argument(s) : none.
*
* Return(s)   : none.
*********************************************************************************************************
*/

void  lpc221x_int_disable (void)
{
    VICIntEnClr = 0xFFFFFFFFL;                                /* Disable ALL interrupts                                   */
}

/*
*********************************************************************************************************
*********************************************************************************************************
**                                     uC/OS-II TIMER FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                            lpc221x_timer_init()
*
* Description : Initialize tick source.
*
* Argument(s) : none.
*
* Return(s)   : none.
*********************************************************************************************************
*/

static void  lpc221x_timer_init (void)
{
#if 0
    uint32_t  pclk_freq;
    uint32_t  tmr_reload;
                                                                /* VIC TIMER #0 Initialization                              */
    VICIntSelect &= ~(1 << VIC_TIMER0);                         /* Enable interrupts                                        */
    VICVectAddr2  = (uint32_t)lpc221x_Tmr_TickISR_Handler;            /* Set the vector address                                   */
    VICVectCntl2  = 0x20 | VIC_TIMER0;                          /* Enable vectored interrupts                               */
    VICIntEnable  = (1 << VIC_TIMER0);                          /* Enable Interrupts                                        */

    pclk_freq     = lpc221x_CPU_PclkFreq();
    tmr_reload    = pclk_freq / OS_TICKS_PER_SEC;
#endif 

	T0IR = 0xFF;                                                /* clear all interrupts */
	T0TCR = 0;                                                  /* Disable timer 0.                                         */
	T0CCR         = 0;                                          /* Capture is disabled.                                     */
	T0EMR         = 0;                                          /* No external match output.                                */
	T0PC          = 0;                                          /* Prescaler is set to no division.                         */
	T0TC = 0;                                                   /* Reset timer counter */
	T0MCR = 0x03;                                               /* Interrupt on MR0; Reset TC on match */
	T0MR0 = (PCLK / TICKS_PER_SECOND); 
	T0TCR = 0x01;                                               /* Enable timer 0                                           */
}



/*
*********************************************************************************************************
*********************************************************************************************************
**                                          LOCAL FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                        lpc221x_pll_init()
*
* Description : Set up and activate the PLL
*
* Argument(s) : none.
*
* Return(s)   : none.
*********************************************************************************************************
*/

static  void  lpc221x_pll_init (void)
{
    uint16_t  loop_ctr;

                                                                /* Configure PLL0, which determines the CPU clock           */
    PLLCFG   = 0x00000023;                                      /* Use PLL values of M = 4 and P = 2                        */
    PLLCON  |= 0x00000001;                                      /* Set the PLL Enable bit                                   */

    
    PLLFEED  = 0xAA;                                            /* Write to the PLL Feed register                           */
    PLLFEED  = 0x55;

    loop_ctr = 10000;                                           /* Wait for the PLL to lock into the new frequency          */
    while (((PLLSTAT & (1<<10)) == 0) && (loop_ctr > 0)) {
        loop_ctr--;
    }

    PLLCON  |= 0x00000002;                                      /* Connect the PLL                                          */

    PLLFEED  = 0xAA;                                            /* Write to the PLL Feed register                           */
    PLLFEED  = 0x55;

    VPBDIV   = 0x00000002;                                      /* Set the VPB frequency to one-half of the CPU clock       */
}

/*
*********************************************************************************************************
*                                             lpc221x_mam_init()
*
* Description : Initialize the memory acceleration module.
*
* Argument(s) : none.
*
* Return(s)   : none.
*********************************************************************************************************
*/

static  void  lpc221x_mam_init (void)
{
    MAMCR  = 0x00;                                              /* Disable the Memory Accelerator Module                    */
    MAMTIM = 0x03;                                              /* MAM fetch cycles are 3 CCLKs in duration                 */
    MAMCR  = 0x02;                                              /* Enable the Memory Accelerator Module                     */
}

/*
*********************************************************************************************************
*********************************************************************************************************
**                                            VIC FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                        Vectored Interrupt Controller
*********************************************************************************************************
*/

static  void  lpc221x_vic_init (void)
{
    VICIntEnClr = 0xFFFFFFFF;                                 /* Disable ALL interrupts                                   */
	lpc221x_delay();
	VICVectAddr = 0;
	VICIntSelect = 0;
    VICProtection = 0;                                          /* Setup interrupt controller                               */
	lpc221x_delay();
}


/*
*********************************************************************************************************
*********************************************************************************************************
**                                            UART FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/

void lpc221x_uart_init(int port, uint32_t baudrate, uint32_t fpclk)
{
	uint16_t Fdiv; 

	if(port == 0)
	{
	    PINSEL0 = (PINSEL0 & 0xfffffff0) | 0x05;    /* Select the pins for Uart0 TXD and RXD */ 
	 
	    U0LCR = 0x80;                               /* Enable to access the frequenc regecter */ 
	    Fdiv = (fpclk / 16) / baudrate;             /* Set the baudrate */ 
	    U0DLM = Fdiv / 256;							 
		U0DLL = Fdiv % 256;						 
	    U0LCR = 0x03;                               /* Disable to access the frequenc regecter */ 
	                                                /* set to 8,1,n */ 
		U0IER = 0x01;                               /* Enable interrupt RBR */ 
	    U0FCR = 0x01;                               /* initial FIFO */ 
	}
	else if(port == 1)
	{
		PINSEL0 = (PINSEL0 & 0xfff0ffff) | 0x50000; /* Select the pins for Uart1 TXD and RXD */ 
 
	    U1LCR = 0x80;                               /* Enable to access the frequenc regecter */ 
	    Fdiv = (fpclk / 16) / baudrate;             /* Set the baudrate */ 
	    U1DLM = Fdiv / 256;							 
		U1DLL = Fdiv % 256;						 
	    U1LCR = 0x03;                               /* Disable to access the frequenc regecter */
	                                                /* set to 8,1,n */
		U1IER = 0x01;                               /* Enable interrupt RBR */
	    U1FCR = 0x01;                               /* initial FIFO */
	}
	
	lpc221x_delay();
}

uint32_t lpc221x_uart_rx_ready(int port)
{
	if(port == 0)
	{
		return ((U0LSR & 0x81) == 0x01);			/* Receiver Data Ready and no Error in RX FIFO */
	}
	else
	{
		return ((U1LSR & 0x81) == 0x01);
	}
}

uint32_t lpc221x_uart_tx_ready(int port)
{
	if(port == 0)
	{
		return (U0LSR & (1<<5));					/* Transmitter Holding Register Empty */
	}
	else
	{
		return (U1LSR & (1<<5));
	}
}

uint8_t lpc221x_uart_rx_byte(int port)
{
	if(port == 0)
		return (U0RBR);
	else
		return (U1RBR);
}

void lpc221x_uart_tx_byte(int port, uint8_t byte)
{
	if(port == 0)
		U0THR = byte;
	else
		U1THR = byte;
}


