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
#include "lwip/mem.h"
#include "lwip/memp.h"
#include "lwip/dhcp.h"
#include "arch/ethernetif.h"
#include "lwip_main.h"
#include "lwip/tcpip.h"
#include <stdio.h>
#include <stdlib.h>
#include "uprintf.h"

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
  struct ip_addr ipaddr;
  struct ip_addr netmask;
  struct ip_addr gw;
  
  /* Create tcp_ip stack thread */
  tcpip_init( NULL, NULL );	

  /* IP address setting & display on STM32_evalboard LCD*/
#ifdef USE_DHCP
  ipaddr.addr = 0;
  netmask.addr = 0;
  gw.addr = 0;

  /* create a timer to start dhcp */
  sys_timeout(2000, lwip_dhcp_start, &xnetif);
#else
  IP4_ADDR(&ipaddr, IP_ADDR0, IP_ADDR1, IP_ADDR2, IP_ADDR3);
  IP4_ADDR(&netmask, NETMASK_ADDR0, NETMASK_ADDR1 , NETMASK_ADDR2, NETMASK_ADDR3);
  IP4_ADDR(&gw, GW_ADDR0, GW_ADDR1, GW_ADDR2, GW_ADDR3);
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

  netif_add(&xnetif, &ipaddr, &netmask, &gw, NULL, &ethernetif_init, &tcpip_input);

 /*  Registers the default network interface. */
  netif_set_default(&xnetif);

 /*  When the netif is fully configured this function must be called.*/
  netif_set_up(&xnetif);

  /* attach ethernet isr */
  bsp_isr_attach(16+ETH_IRQn, ethernetif_isr);
  bsp_irq_unmask(16+ETH_IRQn);

  uprintf_set_enable(UPRINT_INFO, UPRINT_BLK_NET, 1);
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
	struct ip_addr ipaddr;
	struct ip_addr netmask;
	struct ip_addr gw;

	if(pNetif->ip_addr.addr != 0)
	{
	  /* Stop DHCP */
      dhcp_stop(pNetif);

	  uprintf(UPRINT_INFO, UPRINT_BLK_NET, "LWIP DHCP get IP addr: 0x%08x\n", pNetif->ip_addr.addr);
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

		uprintf(UPRINT_INFO, UPRINT_BLK_NET, "LWIP DHCP use static IP addr: 0x%08x\n", pNetif->ip_addr.addr);
      }
	  else
	  {
	  	/* continue to monitor */
	  	sys_timeout(2000, lwip_dhcp_stop, pNetif);
	  }
	}
}

/************************************************
 * for performance measurement
 ************************************************/
typedef struct lwip_perf_cmd_s
{
	uint8_t command;
	uint8_t nDelay;
	uint16_t nPacket;
	uint16_t nSize;
}lwip_perf_cmd_t;

#define PERF_MAX_PAYLOAD_SIZE 1024

static void lwip_perf_thread(void *arg)
{
  static struct netconn *conn;
  static struct netbuf *buf;
  //static ip_addr_t *addr;
  static unsigned short port;
  char *buffer;
  lwip_perf_cmd_t cmd;
  err_t err = ERR_OK;
  uint16_t len, i, j;
  LWIP_UNUSED_ARG(arg);

  conn = netconn_new(NETCONN_UDP);
  LWIP_ASSERT("con != NULL", conn != NULL);
  netconn_bind(conn, NULL, 7); // echo port

  buffer = malloc(PERF_MAX_PAYLOAD_SIZE);
  assert(buffer);
  
  while (1) {
    err = netconn_recv(conn, &buf);
	
    if (err == ERR_OK) {
      //addr = netbuf_fromaddr(buf);
      port = netbuf_fromport(buf);
	  uprintf(UPRINT_INFO, UPRINT_BLK_NET, "LWIP perf Rx: port=%d\n", port);
	  
	  len = netbuf_copy(buf, (char *)&cmd, sizeof(lwip_perf_cmd_t));
	  
      /*  no need netconn_connect here, since the netbuf contains the address */
      if(len != sizeof(lwip_perf_cmd_t)) {
        LWIP_DEBUGF(LWIP_DBG_ON, ("netbuf_copy failed\n"));
      } else {
        /* check packet size */
        if(cmd.nSize>PERF_MAX_PAYLOAD_SIZE || cmd.nSize==0) 
			cmd.nSize = PERF_MAX_PAYLOAD_SIZE;
		/* link buffer to netbuf */
        err = netbuf_ref(buf, buffer, cmd.nSize);
        LWIP_DEBUGF(LWIP_DBG_ON, ("lwip perf: nPacket=%d nSize=%d nDelay=%d\n", cmd.nPacket, cmd.nSize, cmd.nDelay));
        for(i=0; i<cmd.nPacket && err==ERR_OK; i++)
        {
			/* simulate buffer construction */
			for(j=0; j<cmd.nSize; j++)
				buffer[j] = (j&0xFF);
			/* send packet now */
        	err = netconn_send(conn, buf);
	        if(err != ERR_OK) {
	          LWIP_DEBUGF(LWIP_DBG_ON, ("netconn_send failed: %d\n", (int)err));
	        }
			if(cmd.nDelay) task_delay(cmd.nDelay);
        }
		LWIP_DEBUGF(LWIP_DBG_ON, ("lwip perf: send %d packets\n", i));
      }
	  
      netbuf_delete(buf);
    }
  }

  free(buffer);
}

void lwip_perf_init()
{
	sys_thread_new("tlwipp", lwip_perf_thread, NULL, 0x800 /* stack size */, 4 /* priority */);
}

