#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "lwip/opt.h"
#include "lwip/arch.h"
#include "lwip/api.h"
#include "lwip/udp.h"
#include "lwip/netif.h"
#include "ptimer.h"
#include "uprintf.h"
#include "siprintf.h"
#include "hue.h"

static uint32_t ipMultiCast;
struct udp_pcb *g_ssdp_upcb;
static ptimer_t ssdp_timer;

#define SSDP_PACKET_MAXSIZE 512
static uint8_t ssdp_packet[SSDP_PACKET_MAXSIZE];
static char str_ssdp_location[64];
static char str_ssdp_uuid[64];

extern ptimer_table_t g_zll_timer_table;
const char *str_ssdp_notify = "NOTIFY * HTTP/1.1\r\n";
const char *str_ssdp_header_host = "HOST: 239.255.255.250:1900\r\n";
const char *str_ssdp_header_cache = "CACHE-CONTROL: max-age=100\r\n";
const char *str_ssdp_header_server = "SERVER: Probability/0.40, UPnP/1.0, IpBridge/0.1\r\n";
const char *str_ssdp_header_nts = "NTS: ssdp:alive\r\n";
const char *str_ssdp_header_nt = "NT: upnp:rootdevice\r\n";
const char *str_ssdp_header_nt2 = "NT: urn:schemas-upnp-org:device:basic:1\r\n";

#define SSDP_PORT 1900

#define SSDP_INTERVAL_REPEAT  5 //50ms
#define SSDP_INTERVAL_MESSAGE 10 //100ms
#define SSDP_INTERVAL_CYCLE   1000 //10s

#define SSDP_STATE_00 0
#define SSDP_STATE_01 1
#define SSDP_STATE_10 2
#define SSDP_STATE_11 3
#define SSDP_STATE_20 4
#define SSDP_STATE_21 5

int ssdp_transmit(uint16_t remote_port, uint8_t *data, uint16_t len);

/* send SSDP alive packet related possible code:
#define LWIP_IGMP                       1

// in
netif->flags |= NETIF_FLAG_IGMP;

*/

/* build location string from ip address */
uint32_t ssdp_build_location(char *outstr, uint32_t size)
{
    uint32_t offset = 0, ip;
    uint8_t *pIp;

    *outstr = '\0';
    if(netif_default)
    {
        /* build location */
        ip = ntohl(netif_default->ip_addr.addr);
        pIp = (uint8_t *)&ip;
        offset += siprintf(outstr, size, "LOCATION: http://%d.%d.%d.%d:80/\r\n", pIp[0], pIp[1], pIp[2], pIp[3]);
    }

    return offset;
}

/* build UUID from MAC address in form of 2f402f80-da50-1111-9933-00118877ff55 */
uint32_t ssdp_build_uuid(char *outstr, uint32_t size)
{
    uint32_t offset = 0;
    uint8_t *mac;

    *outstr = '\0';
    if(netif_default)
    {
        /* build location */
        mac = netif_default->hwaddr;
        offset += siprintf(outstr, size, "2f402f80-da50-1111-9933-%02x%02x%02x%02x%02x%02x/\r\n", 
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    }

    return offset;
}

uint32_t ssdp_timeout(void *tim, uint32_t param1, uint32_t param2)
{
    int ret, offset = 0;
    uint16_t state = SSDP_STATE_00, interval = SSDP_INTERVAL_CYCLE; 
    ptimer_t *ptimer = (ptimer_t *)tim;
    char *outp = (char *)ssdp_packet;

    switch(param1) //state
    {
        case SSDP_STATE_00:
            state = SSDP_STATE_01;
            interval = SSDP_INTERVAL_REPEAT;
            ssdp_build_location(str_ssdp_location, 64);
            ssdp_build_uuid(str_ssdp_uuid, 64);
            /* USN: uuid:2f402f80-da50-11e1-9b23-0017881733fb::upnp:rootdevice\r\n */
            siprintf(outp, SSDP_PACKET_MAXSIZE-offset, "%s%s%s%s%s%s%sUSN: uuid:%s::upnp:rootdevice\r\n\r\n",
                    str_ssdp_notify, str_ssdp_header_host, str_ssdp_header_cache,
                    str_ssdp_location, str_ssdp_header_server, str_ssdp_header_nts,
                    str_ssdp_header_nt, str_ssdp_uuid);
            break;

        case SSDP_STATE_01:
            state = SSDP_STATE_10;
            interval = SSDP_INTERVAL_MESSAGE;
            break;

        case SSDP_STATE_10:
            state = SSDP_STATE_11;
            interval = SSDP_INTERVAL_REPEAT;
            ssdp_build_location(str_ssdp_location, 64);
            ssdp_build_uuid(str_ssdp_uuid, 64);
            /* USN: uuid:2f402f80-da50-11e1-9b23-0017881733fb\r\n */
            siprintf(outp, SSDP_PACKET_MAXSIZE-offset, "%s%s%s%s%s%sNT: uuid:%s\r\nUSN: uuid:%s\r\n\r\n",
                    str_ssdp_notify, str_ssdp_header_host, str_ssdp_header_cache,
                    str_ssdp_location, str_ssdp_header_server, str_ssdp_header_nts,
                    str_ssdp_uuid, str_ssdp_uuid);
            break;

        case SSDP_STATE_11:
            state = SSDP_STATE_20;
            interval = SSDP_INTERVAL_MESSAGE;
            break;

        case SSDP_STATE_20:
            state = SSDP_STATE_21;
            interval = SSDP_INTERVAL_REPEAT;
            ssdp_build_location(str_ssdp_location, 64);
            ssdp_build_uuid(str_ssdp_uuid, 64);
            siprintf(outp, SSDP_PACKET_MAXSIZE-offset, "%s%s%s%s%s%s%sUSN: uuid:%s\r\n\r\n",
                    str_ssdp_notify, str_ssdp_header_host, str_ssdp_header_cache,
                    str_ssdp_location, str_ssdp_header_server, str_ssdp_header_nts,
                    str_ssdp_header_nt2, str_ssdp_uuid);
            break;

        case SSDP_STATE_21:
            state = SSDP_STATE_00;
            interval = SSDP_INTERVAL_CYCLE;
            break;
    }

    /* send ssdp alive notify */
    ret = ssdp_transmit(SSDP_PORT, (uint8_t *)ssdp_packet, strlen((char *)ssdp_packet));
    if(ret >= 0)
    {
        /* update the new state and start timer again */
        ptimer->param[0] = state;
        ptimer_start(&g_zll_timer_table, ptimer, interval);
    }
    else
        uprintf(UPRINT_WARNING, UPRINT_BLK_HUE, "failed to send SSDP alive: %d\n", ret);

    return ret;
}

/* as it doesn't recv multicast packet, we don't initialize the rx part */
int ssdp_init()
{
	int iRet = 1;
	uint16_t localPort = SSDP_PORT;
	uint16_t remotePort = SSDP_PORT;

	ipMultiCast = htonl(0xEFFFFFFA); //239.255.255.250
    g_ssdp_upcb = udp_new();
	assert(g_ssdp_upcb);
	
    ssdp_timer.flags = 0;
	ssdp_timer.onexpired_func = ssdp_timeout;
    ssdp_timer.param[0] = SSDP_STATE_00;
    ssdp_timer.param[1] = 0;
	iRet = ptimer_start(&g_zll_timer_table, &ssdp_timer, SSDP_INTERVAL_CYCLE);
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



