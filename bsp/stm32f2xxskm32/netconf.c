/**
  ******************************************************************************
  * @file    netconf.c
  * @author  MCD Application Team
  * @version V1.1.0
  * @date    07-October-2011
  * @brief   Network connection configuration
  ******************************************************************************
  * @attention
  *
  * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
  * TIME. AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY
  * DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
  * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
  * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  * <h2><center>&copy; COPYRIGHT 2011 STMicroelectronics</center></h2>
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>

#include "lwip_main.h"
#include "lwip/opt.h"
#include "lwip/mem.h"
#include "lwip/memp.h"
#include "lwip/ip_addr.h"
#include "lwip/dhcp.h"
#include "lwip/tcpip.h"
#include "lwip/timers.h"
#include "ethernetif.h"

#include "uprintf.h"
#include "cmsis_os.h"

/* Private typedef -----------------------------------------------------------*/
typedef enum 
{ 
  DHCP_START=0,
  DHCP_WAIT_ADDRESS,
  DHCP_ADDRESS_ASSIGNED,
  DHCP_TIMEOUT
} 
DHCP_State_TypeDef;
/* Private define ------------------------------------------------------------*/
#define MAX_DHCP_TRIES 5

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
struct netif xnetif; /* network interface structure */

/* Private functions ---------------------------------------------------------*/
void lwip_dhcp_start(void *pParameter);
void lwip_dhcp_stop(void *pParameter);


extern void ethernetif_isr(int irq);

/**
  * @brief  Initializes the lwIP stack
  * @param  None
  * @retval None
  */
void lwip_sys_init(void)
{
  ip_addr_t ipaddr;
  ip_addr_t netmask;
  ip_addr_t gw;

  uprintf_set_enable(UPRINT_INFO, UPRINT_BLK_NET, 1);
  uprintf_set_enable(UPRINT_DEBUG, UPRINT_BLK_NET, 0);
  
  /* Create tcp_ip stack thread */
  tcpip_init( NULL, NULL );	

  /* IP address setting */
  IP4_ADDR(&ipaddr, IP_ADDR0, IP_ADDR1, IP_ADDR2, IP_ADDR3);
  IP4_ADDR(&netmask, NETMASK_ADDR0, NETMASK_ADDR1 , NETMASK_ADDR2, NETMASK_ADDR3);
  IP4_ADDR(&gw, GW_ADDR0, GW_ADDR1, GW_ADDR2, GW_ADDR3);

#ifdef USE_DHCP
  /* create a timer to start dhcp */
	sys_timeout(2000, lwip_dhcp_start, &xnetif);
#else
  uprintf(UPRINT_INFO, UPRINT_BLK_NET, "LWIP eth0 static IP: %d.%d.%d.%d\n", IP_ADDR0, IP_ADDR1, IP_ADDR2, IP_ADDR3);
#endif

  /* - netif_add(struct netif *netif, struct ip_addr *ipaddr,
            struct ip_addr *netmask, struct ip_addr *gw,
            void *state, err_t (* init)(struct netif *netif),
            err_t (* input)(struct pbuf *p, struct netif *netif))
    
   Adds your network interface to the netif_list. Allocate a struct
  netif and pass a pointer to this structure as the first argument.
  Give pointers to cleared ip_addr structures when using DHCP,
  or fill them with sane numbers otherwise. The state pointer may be NULL.

  The init function pointer must point to a initialization function for
  your ethernet netif interface. The following code illustrates it's use.*/
  xnetif.flags = NETIF_FLAG_IGMP; // enable multicast
  netif_add(&xnetif, &ipaddr, &netmask, &gw, NULL, &ethernetif_init, &tcpip_input);

 /*  Registers the default network interface. */
  netif_set_default(&xnetif);

 /*  When the netif is fully configured this function must be called.*/
  netif_set_up(&xnetif);

 /* When to set link up for eth if? we don' have link status interrupt. */
  netif_set_link_up(&xnetif);

  uprintf(UPRINT_INFO, UPRINT_BLK_NET, "ethernet netif is up.\n");

  #ifdef ETH_LINK_DETECT
  /* Set the link callback function, this function is called on change of link status*/
  netif_set_link_callback(&xnetif, ethernetif_update_config);
  
  /* create a binary semaphore used for informing ethernetif of frame reception */
  osSemaphoreDef(Netif_SEM);
  Netif_LinkSemaphore = osSemaphoreCreate(osSemaphore(Netif_SEM) , 1 );
  
  link_arg.netif = &xnetif;
  link_arg.semaphore = Netif_LinkSemaphore;
  /* Create the Ethernet link handler thread */
  osThreadDef(LinkThr, ethernetif_set_link, osPriorityNormal, 0, configMINIMAL_STACK_SIZE);
  osThreadCreate (osThread(LinkThr), &link_arg);
  #endif //ETH_LINK_DETECT
  /* attach ethernet isr */
  bsp_isr_attach(16+ETH_IRQn, ethernetif_isr);
  bsp_irq_unmask(16+ETH_IRQn);
}

void lwip_dhcp_start(void *pParameter)
{
	struct netif *pNetif = (struct netif *)pParameter;
	
	dhcp_start(pNetif);
	sys_timeout(2000, lwip_dhcp_stop, pNetif);

	uprintf(UPRINT_INFO, UPRINT_BLK_NET, "LWIP DHCP started\n");
}

void lwip_dhcp_stop(void *pParameter)
{
	struct netif *pNetif = (struct netif *)pParameter;
	ip_addr_t ipaddr;
	ip_addr_t netmask;
	ip_addr_t gw;

	if(pNetif->ip_addr.addr != 0)
	{
	  /* Stop DHCP */
      dhcp_stop(pNetif);

	  uprintf(UPRINT_INFO, UPRINT_BLK_NET, "LWIP DHCP get IP addr: %d.%d.%d.%d\n", 
	  		(pNetif->ip_addr.addr&0xFF000000)>>24,
	  		(pNetif->ip_addr.addr&0x00FF0000)>>16,
	  		(pNetif->ip_addr.addr&0x0000FF00)>>8,
	  		(pNetif->ip_addr.addr&0x000000FF));
	}
	else
	{
	  /* DHCP timeout */
      if (pNetif->dhcp->tries > MAX_DHCP_TRIES)
      {

        /* Stop DHCP */
        dhcp_stop(pNetif);

        /* Static address used */
        IP4_ADDR(&ipaddr, IP_ADDR0 ,IP_ADDR1 , IP_ADDR2 , IP_ADDR3 );
        IP4_ADDR(&netmask, NETMASK_ADDR0, NETMASK_ADDR1, NETMASK_ADDR2, NETMASK_ADDR3);
        IP4_ADDR(&gw, GW_ADDR0, GW_ADDR1, GW_ADDR2, GW_ADDR3);
        netif_set_addr(pNetif, &ipaddr , &netmask, &gw);

		uprintf(UPRINT_INFO, UPRINT_BLK_NET, "LWIP DHCP use static IP addr: %d.%d.%d.%d\n", 
				(pNetif->ip_addr.addr&0xFF000000)>>24,
		  		(pNetif->ip_addr.addr&0x00FF0000)>>16,
		  		(pNetif->ip_addr.addr&0x0000FF00)>>8,
		  		(pNetif->ip_addr.addr&0x000000FF));
      }
	  else
	  {
	  	/* continue to monitor */
	  	sys_timeout(2000, lwip_dhcp_stop, pNetif);
	  }
	}
}


