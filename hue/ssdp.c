#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "lwip/opt.h"
#include "lwip/arch.h"
#include "lwip/api.h"
#include "lwip/udp.h"
#include "lwip/igmp.h"

#include "ipc.h"
#include "ptimer.h"
#include "uprintf.h"
#include "siprintf.h"
#include "hue.h"

struct udp_pcb *g_ssdp_upcb;
static ptimer_t ssdp_timer;

#define SSDP_PACKET_MAXSIZE 512
static uint8_t ssdp_packet[SSDP_PACKET_MAXSIZE];
static char str_ssdp_ipaddr[32];
static char str_ssdp_uuid[64];
extern mbox_t g_hue_mbox;
extern ptimer_table_t g_zll_timer_table;
const char *str_ssdp_notify = "NOTIFY * HTTP/1.1\r\n";
const char *str_ssdp_header_host = "HOST: 239.255.255.250:1900\r\n";
const char *str_ssdp_header_cache = "CACHE-CONTROL: max-age=100\r\n";
const char *str_ssdp_header_server = "SERVER: Probability/0.40, UPnP/1.0, IpBridge/0.1\r\n";
const char *str_ssdp_header_nts = "NTS: ssdp:alive\r\n";
const char *str_ssdp_header_nt = "NT: upnp:rootdevice\r\n";
const char *str_ssdp_header_nt2 = "NT: urn:schemas-upnp-org:device:basic:1\r\n";

const char *str_ssdp_ok = "HTTP/1.1 200 OK\r\n";
const char *str_ssdp_header_st = "ST: upnp:rootdevice\r\n";
const char *str_ssdp_header_st2 = "ST: urn:schemas-upnp-org:device:basic:1\r\n";


#define SSDP_PORT 1900
#define SSDP_IPADDR 0xEFFFFFFA // 239.255.255.250


#define SSDP_INTERVAL_REPEAT  10 //50ms
#define SSDP_INTERVAL_MESSAGE 10 //100ms
#define SSDP_INTERVAL_CYCLE   1000 //10s

#define SSDP_STATE_00 0
#define SSDP_STATE_01 1
#define SSDP_STATE_10 2
#define SSDP_STATE_11 3
#define SSDP_STATE_20 4
#define SSDP_STATE_21 5

extern uint32_t http_build_ipaddr(char *outstr, uint32_t size);
extern uint32_t http_build_uuid(char *outstr, uint32_t size);

/* send SSDP alive packet related possible code:
#define LWIP_IGMP                       1

// in
netif->flags |= NETIF_FLAG_IGMP;

*/

// send a ssdp packet: multicast or unicast.
// etharp_output will transfer multicast IP addr to multicast MAC addr
int ssdp_transmit(uint32_t remote_ip, uint16_t remote_port, uint8_t *data, uint16_t len) 
{
    int ret=-2;
    struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT,len, PBUF_RAM);

    remote_ip = htonl(remote_ip);
	assert(p);
    pbuf_take(p, data, len);
    ret = udp_sendto(g_ssdp_upcb, p,(struct ip_addr *) &remote_ip,remote_port);
    pbuf_free(p);

    return ret;
}


uint32_t ssdp_timeout(void *tim, uint32_t param1, uint32_t param2)
{
    int ret, isMulticase = 1;
    uint16_t new_state = SSDP_STATE_00, state = (param1>>16), interval = SSDP_INTERVAL_CYCLE;
    uint16_t port = (param1 & 0xFFFF);
    uint32_t ip = param2; // param2 is value of ptimer->param[1]
    
    ptimer_t *ptimer = (ptimer_t *)tim;
    char *outp = (char *)ssdp_packet;

    if(ip != SSDP_IPADDR)
    {
        uprintf(UPRINT_INFO, UPRINT_BLK_HUE, "SSDP state %d transmit to ip=0x%08x port=%d\n", state, ip, port);
        isMulticase = 0;
    }

    switch(state)
    {
        case SSDP_STATE_00:
            new_state = SSDP_STATE_01;
            interval = SSDP_INTERVAL_REPEAT;
            http_build_ipaddr(str_ssdp_ipaddr, 32);
            http_build_uuid(str_ssdp_uuid, 64);
            /* USN: uuid:2f402f80-da50-11e1-9b23-0017881733fb::upnp:rootdevice\r\n */
            if(isMulticase)
                siprintf(outp, SSDP_PACKET_MAXSIZE, "%s%s%sLOCATION: http://%s:80/\r\n%s%s%sUSN: uuid:%s::upnp:rootdevice\r\n\r\n",
                        str_ssdp_notify, str_ssdp_header_host, str_ssdp_header_cache,
                        str_ssdp_ipaddr, str_ssdp_header_server, str_ssdp_header_nts,
                        str_ssdp_header_nt, str_ssdp_uuid);
            else
                siprintf(outp, SSDP_PACKET_MAXSIZE, "%s%sEXT:\r\nLOCATION: http://%s:80/description.xml\r\n%s%sUSN: uuid:%s::upnp:rootdevice\r\n\r\n", 
                        str_ssdp_ok, str_ssdp_header_cache, str_ssdp_ipaddr,
                        str_ssdp_header_server, str_ssdp_header_st, str_ssdp_uuid);
            break;

        case SSDP_STATE_01:
            new_state = SSDP_STATE_10;
            interval = SSDP_INTERVAL_REPEAT;
            break;

        case SSDP_STATE_10:
            new_state = SSDP_STATE_11;
            interval = SSDP_INTERVAL_REPEAT;
            http_build_ipaddr(str_ssdp_ipaddr, 32);
            http_build_uuid(str_ssdp_uuid, 64);
            /* USN: uuid:2f402f80-da50-11e1-9b23-0017881733fb\r\n */
            if(isMulticase)
                siprintf(outp, SSDP_PACKET_MAXSIZE, "%s%s%sLOCATION: http://%s:80/\r\n%s%sNT: uuid:%s\r\nUSN: uuid:%s\r\n\r\n",
                        str_ssdp_notify, str_ssdp_header_host, str_ssdp_header_cache,
                        str_ssdp_ipaddr, str_ssdp_header_server, str_ssdp_header_nts,
                        str_ssdp_uuid, str_ssdp_uuid);
            else
                siprintf(outp, SSDP_PACKET_MAXSIZE, "%s%sEXT:\r\nLOCATION: http://%s:80/description.xml\r\n%sST: uuid:%s\r\nUSN: uuid:%s\r\n\r\n", 
                        str_ssdp_ok, str_ssdp_header_cache, str_ssdp_ipaddr,
                        str_ssdp_header_server, str_ssdp_uuid, str_ssdp_uuid);
            break;

        case SSDP_STATE_11:
            new_state = SSDP_STATE_20;
            interval = SSDP_INTERVAL_REPEAT;
            break;

        case SSDP_STATE_20:
            new_state = SSDP_STATE_21;
            interval = SSDP_INTERVAL_REPEAT;
            http_build_ipaddr(str_ssdp_ipaddr, 32);
            http_build_uuid(str_ssdp_uuid, 64);
            if(isMulticase)
                siprintf(outp, SSDP_PACKET_MAXSIZE, "%s%s%sLOCATION: http://%s:80/\r\n%s%s%sUSN: uuid:%s\r\n\r\n",
                        str_ssdp_notify, str_ssdp_header_host, str_ssdp_header_cache,
                        str_ssdp_ipaddr, str_ssdp_header_server, str_ssdp_header_nts,
                        str_ssdp_header_nt2, str_ssdp_uuid);
            else
                siprintf(outp, SSDP_PACKET_MAXSIZE, "%s%sEXT:\r\nLOCATION: http://%s:80/description.xml\r\n%s%sUSN: uuid:%s\r\n\r\n", 
                        str_ssdp_ok, str_ssdp_header_cache, str_ssdp_ipaddr,
                        str_ssdp_header_server, str_ssdp_header_st2, str_ssdp_uuid);
            break;

        case SSDP_STATE_21:
            new_state = SSDP_STATE_00;
            interval = SSDP_INTERVAL_CYCLE;
            ptimer->param[1] = SSDP_IPADDR;             // re-switch to multicast 
            ptimer->param[0] = SSDP_PORT;
            break;
    }
    
    if(!isMulticase)
    {
        task_delay(5);
        kprintf("SSDP Tx: %s\n", outp);
    }

    /* send ssdp alive notify */
    //uprintf(UPRINT_DEBUG, UPRINT_BLK_HUE, "SSDP: state=%d\n", state);
    ret = ssdp_transmit(ip, port, (uint8_t *)ssdp_packet, strlen((char *)ssdp_packet));
    if(ret >= 0)
    {
        /* update the new state and start timer again */
        ptimer->param[0] = (new_state<<16) | port;
        ptimer_start(&g_zll_timer_table, ptimer, interval);
    }
    else
        uprintf(UPRINT_WARNING, UPRINT_BLK_HUE, "failed to send SSDP alive: %d\n", ret);

    return ret;
}


/* UDP receive .............................................................*/
void ssdp_recv(void *arg, struct udp_pcb *upcb,
               struct pbuf *p, struct ip_addr *addr, u16_t port)
{
    /* process the payload in p->payload */
    char ssdp_discovery[230];
    uint16_t len;
    hue_mail_t huemail;

    /* copy to a local buffer */
    len = pbuf_copy_partial(p, ssdp_discovery, 230-1 /* len */, 0 /* offset */);
    ssdp_discovery[len] = '\0';

    pbuf_free(p);   /* don't leak the pbuf! */

    if(strstr(ssdp_discovery, "ssdp:discover"))
    {
        uprintf(UPRINT_INFO, UPRINT_BLK_HUE, "Rx SSDP packet: port=%d size=%d content:\n", port, p->tot_len);
        uprintf(UPRINT_INFO, UPRINT_BLK_HUE, "%s\n", ssdp_discovery);

        /* send a mail to re-start SSDP state machine */
        huemail.ssdp.cmd = HUE_MAIL_CMD_SSDP;
        huemail.ssdp.interval = 3;
        huemail.ssdp.port = port;
        huemail.ssdp.ipaddr = ntohl(addr->addr);

        mbox_post(&g_hue_mbox, (uint32_t *)&huemail);
    }
}

/* init or re-init ssdp state machine 
 * destIp: IP addr in host byte order
 * destPort: UDP port in host byte order
 * interval: interval to start state machine for SSDP transfer (in 10 ms)
 */
void ssdp_statemachine_init(uint32_t destIp, uint32_t destPort, uint16_t interval)
{
    int ret;

    if(ptimer_is_running(&ssdp_timer))
        ptimer_cancel(&g_zll_timer_table, &ssdp_timer);

    ssdp_timer.flags = 0;
	ssdp_timer.onexpired_func = ssdp_timeout;
    ssdp_timer.param[0] = (SSDP_STATE_00 << 16) | destPort;
    ssdp_timer.param[1] = destIp;
	ret = ptimer_start(&g_zll_timer_table, &ssdp_timer, interval);
	assert(ret == 0);
}

int ssdp_init()
{
	int iRet = 1;
	uint16_t localPort = SSDP_PORT;
    uint32_t ipMultiCast;

	ipMultiCast = htonl(SSDP_IPADDR);
    g_ssdp_upcb = udp_new();
	assert(g_ssdp_upcb);
	
    ssdp_statemachine_init(SSDP_IPADDR, SSDP_PORT, SSDP_INTERVAL_CYCLE);
	
    iRet = igmp_joingroup(IP_ADDR_ANY,(struct ip_addr *) (&ipMultiCast));
    if(iRet == ERR_OK)
    {
	    iRet = udp_bind(g_ssdp_upcb, IP_ADDR_ANY, localPort);
        udp_recv(g_ssdp_upcb, &ssdp_recv, (void *)0);
    }
    else
        uprintf(UPRINT_WARNING, UPRINT_BLK_HUE, "igmp_joingroup failed: %d\n", iRet);
	//iRet = udp_connect(g_ssdp_upcb,IP_ADDR_ANY, remotePort);
    return iRet; 
}




