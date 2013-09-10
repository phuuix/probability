/*
 * icmp.c
 */
#include <sys/types.h>
#include <bsp.h>
#include <net/in.h>
#include <net/ip.h>
#include <net/icmp.h>

#include "tinyip.h"

void tinyip_icmp_init()
{
}


int tinyip_icmp_input(struct tinyip_buf *pkt)
{
	struct iphdr *ip;
	struct icmphdr *icmp;
	char *buf;
	char *macaddress;
	uint32_t ipaddress;

	ip = (struct iphdr *)(pkt->stacks[pkt->sp-1]);
	icmp = (struct icmphdr *)(pkt->stacks[pkt->sp]);
	if(icmp->type == ICMP_ECHO)	/* echo request */
	{
		uint8_t tmpchar;

		macaddress = tinyip_ctrl.macaddr;
		ipaddress = tinyip_ctrl.ipaddr;

		/* adjust ethernet header */
		tinyip_memcpy(buf, buf+6, 6);
		tinyip_memcpy(buf+6, macaddress, 6);

		/* adjust ip header */
		ip->daddr = ip->saddr;
		ip->saddr = tinyip_htonl(ipaddress);

		/* adjust icmp packet */
		tmpchar = ICMP_ECHOREPLY;
		tinyip_checksum_fixup((uint8_t *)&icmp->checksum, (uint8_t *)&icmp->type, 1, (uint8_t *)&tmpchar, 1);
		icmp->type = ICMP_ECHOREPLY;

		/* send packet */
		return tinyip_ip_output(pkt, ipaddress, tinyip_ntohl(ip->daddr), IPPROTO_ICMP);
	}

	return 0;
}


int tinyip_icmp_output(struct tinyip_buf *pkt)
{
	return 0;
}
