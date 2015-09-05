#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "lwip/opt.h"
#include "lwip/arch.h"
#include "lwip/api.h"
#include "lwip/udp.h"
#include "ptimer.h"
#include "uprintf.h"
#include "hue.h"

static uint32_t ipMultiCast;
struct udp_pcb *g_ssdp_upcb;
static ptimer_t ssdp_timer;

extern ptimer_table_t g_zll_timer_table;
const char *str_ssdp_alive = "NOTIFY * HTTP/1.1\r\nHOST: 239.255.255.250:1900\r\nCACHE-CONTROL: max-age=100\r\nLOCATION: http://192.168.2.10:80/\r\nSERVER: Probability/0.40, UPnP/1.0, IpBridge/0.1\r\nNTS: ssdp:alive\r\nNT: upnp:rootdevice\r\nUSN: uuid:2f402f80-da50-1111-9933-00118877ff55::upnp:rootdevice\r\n\r\n";

#define SSDP_PORT 1900

/* send SSDP alive packet related possible code:
#define LWIP_IGMP                       1

// in
netif->flags |= NETIF_FLAG_IGMP;

*/
int ssdp_transmit_alive();

uint32_t ssdp_timeout(void *tim, uint32_t param1, uint32_t param2)
{
    int ret;
    ptimer_t *timer = (ptimer_t *)tim;

    ret = ssdp_transmit_alive();
    if(ret > 0)
    {
        ptimer_start(&g_zll_timer_table, timer, timer->duration);
    }
    else
        uprintf(UPRINT_WARNING, UPRINT_BLK_HUE, "failed to send SSDP alive\n");

    return ret;
}

/* as don't recv, we don't initialize the rx part */
int ssdp_init()
{
	int iRet = 1;
	uint16_t localPort = SSDP_PORT;
	uint16_t remotePort = SSDP_PORT;
	
	ipMultiCast = 0xEFFFFFFA; //239.255.255.250
    g_ssdp_upcb = udp_new();
	assert(g_ssdp_upcb);
	
    ssdp_timer.flags = 0;
	ssdp_timer.onexpired_func = ssdp_timeout;
    ssdp_timer.param[0] = 0;
    ssdp_timer.param[1] = 0;
	iRet = ptimer_start(&g_zll_timer_table, &ssdp_timer, 90*100);
	assert(iRet == 0);
	
//  iRet = igmp_joingroup(IP_ADDR_ANY,(struct ip_addr *) (&ipMultiCast));
	udp_bind(g_ssdp_upcb, IP_ADDR_ANY, localPort);
//	udp_recv(g_upcb, &my_udp_rx, (void *)0);
	iRet = udp_connect(g_ssdp_upcb,IP_ADDR_ANY, remotePort);
    return iRet; 
}

// I think the below function will work if you want to only send some multicast packets.
// Because etharp_output will transfer multicast IP addr to multicast MAC addr
int ssdp_transmit(uint16_t remote_port, uint8_t *data, uint16_t len) 
{
    int ret=-2;
    struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT,len, PBUF_RAM);
	assert(p);
    memcpy(p->payload, data, len);
    ret = udp_sendto(g_ssdp_upcb, p,(struct ip_addr *) (&ipMultiCast),remote_port);
    pbuf_free(p);

    return ret;
}

int ssdp_transmit_alive()
{
	return ssdp_transmit(SSDP_PORT, (uint8_t *)str_ssdp_alive, strlen(str_ssdp_alive));
}
