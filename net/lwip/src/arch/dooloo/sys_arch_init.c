/*
 * sys_arch_init.c
 * init file for lwip.
 */

#include <stdio.h>
#include "lwip/debug.h"
#include "lwip/mem.h"
#include "lwip/memp.h"
#include "lwip/sys.h"
#include "lwip/stats.h"
#include "lwip/tcpip.h"
#include "lwip/ip_addr.h"

//#include "netif/ethernetif.h"
#include "netif/etharp.h"

#include <net/netintf.h>

#define RTL8139
//#define S3C4510

#ifdef RTL8139
struct netif rtl8139if;
extern struct ifnet rtl8139_ifnet;
#endif /* RTL8139 */

#ifdef S3C4510
struct netif s3c4510if;
extern struct ifnet s3c4510_ifnet;
#endif /* S3C4510 */

extern err_t ethernetif_input(struct pbuf *p, struct netif *netif);
extern err_t ethernetif_output(struct netif *netif, struct pbuf *p, struct ip_addr *ipaddr);

static void arp_timer(void *arg)
{
	etharp_tmr();
	sys_timeout(ARP_TMR_INTERVAL, (sys_timeout_handler)arp_timer, NULL);
}

#ifdef RTL8139
static err_t lwip_rtl8139_linkoutput(struct netif *netif, struct pbuf *p)
{
	rtl8139_ifnet.if_send(0, p, 0);

/*
#ifdef LINK_STATS
	stats.link.xmit++;
#endif
*/
	return ERR_OK;
}

err_t lwip_rtl8139_init(struct netif *netif)
{
	extern struct ifnet rtl8139_ifnet;
	
	netif->state = (void *)&rtl8139_ifnet;
	netif->name[0] = 'e';
	netif->name[1] = '0';
	netif->hwaddr_len = 6;
	netif->output = ethernetif_output;
	netif->linkoutput = lwip_rtl8139_linkoutput;
	rtl8139_ifnet.if_ioctl(0, NIOCTL_GADDR, netif->hwaddr);

	return ERR_OK;
}
#endif /* RTL8139 */

#ifdef S3C4510
static err_t lwip_s3c4510_linkoutput(struct netif* netif, struct pbuf* p)
{
	s3c4510_ifnet.if_send(0, p, 0);

	return ERR_OK;
}

err_t lwip_s3c4510_init(struct netif *netif)
{
	extern struct ifnet s3c4510_ifnet;
	
	netif->state = (void *)&s3c4510_ifnet;
	netif->name[0] = 'e';
	netif->name[1] = '0';
	netif->hwaddr_len = 6;
	netif->output = ethernetif_output;
	netif->linkoutput = lwip_s3c4510_linkoutput;
	s3c4510_ifnet.if_ioctl(0, NIOCTL_GADDR, netif->hwaddr);

	netif_set_up(netif);

	return ERR_OK;
}
#endif

/*
 * lwip system initial entry
 */
void lwip_sys_init()
{
	struct ip_addr ipaddr, netmask, gw;

	#ifdef STATS
	stats_init();
	#endif /* STATS */

	sys_init();

	mem_init();
	memp_init();
	pbuf_init();
    
	IP4_ADDR(&gw, 0,0,0,0);
	IP4_ADDR(&ipaddr, 192,88,88,100);
	IP4_ADDR(&netmask, 255,255,255,0);

#ifdef RTL8139
	netif_add(&rtl8139if, &ipaddr, &netmask, &gw, NULL, 
		lwip_rtl8139_init, ethernetif_input);
	netif_set_default(&rtl8139if);

//	dhcp_start(&rtl8139if);
//	nat_init(&rtl8139if, tcpip_input);
//	nat_netif_add(NULL, &rtl8139if);
#endif

#ifdef S3C4510
	netif_add(&s3c4510if, &ipaddr, &netmask, &gw, NULL, 
		lwip_s3c4510_init, ethernetif_input);
	netif_set_default(&s3c4510if);

//	dhcp_start(&s3c4510if);
//	nat_init(&s3c4510if, tcpip_input);
//	nat_netif_add(NULL, &s3c4510if);
#endif

	etharp_init();
	sys_timeout(ARP_TMR_INTERVAL, (sys_timeout_handler)arp_timer, NULL);
	tcpip_init(NULL, NULL);
}

#ifdef SHELL_COMMAND
#include <shell.h>

/*
 * set netif ip address
 */
int netif_ip_set(struct shell_session *ss, int argc, char **argv)
{
	char *pip, *pmask;
	u_long ip, mask;

	pip = argv[3];
	pmask = argv[4];
	ip = inet_addr(pip);
	mask = inet_addr(pmask);
	netif_set_ipaddr(netif_default, (struct ip_addr *)&ip);
	netif_set_netmask(netif_default, (struct ip_addr *)&mask);
	
	return ROK;
}

/*
 * show netif ip address
 */
int netif_ip_show(struct shell_session *ss, int argc, char **argv)
{
	u_long ip, mask;
	u_char *pip, *pmask;

	ip = netif_default->ip_addr.addr;
	mask = netif_default->netmask.addr;
	pip = (u_char *)&ip;
	pmask = (u_char *)&mask;
	
	ss->output(" netif ip address: %u.%u.%u.%u\n", pip[0], pip[1], pip[2], pip[3]);
	ss->output(" netif net mask: %u.%u.%u.%u\n", pmask[0], pmask[1], pmask[2], pmask[3]);
	
	return ROK;
}
#endif /* SHELL_COMMAND */
