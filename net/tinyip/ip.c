/*
 * ip.c
 */
#include <sys/types.h>
#include <bsp.h>
#include <net/in.h>
#include <net/ip.h>
#include <net/if_ether.h>

#include "tinyip.h"

void tinyip_ip_init()
{
}

int tinyip_ip_input(struct tinyip_buf *pkt)
{
	int ret = 0;
	struct iphdr *ip;

	ip = (struct iphdr *)(pkt->stacks[pkt->sp]);

	if(tinyip_ntohl(ip->daddr) != tinyip_ctrl.ipaddr)
	{
		if(pkt->flags & TINYIP_FLAG_OUTSIDE)
			tinyip_ctrl.buf_free(pkt);
		return 0;
	}

	pkt->sp ++;
	pkt->stacks[pkt->sp] = (char *)(ip + 1);
	pkt->length -= sizeof(struct iphdr);

	switch(ip->protocol)
	{
		#ifdef TINYIP_INCLUDE_ICMP
		case IPPROTO_ICMP:
			ret = tinyip_icmp_input(pkt);
			break;
		#endif

		case IPPROTO_UDP:
			ret = tinyip_udp_input(pkt);
			break;

		default:
			if(pkt->flags & TINYIP_FLAG_OUTSIDE)
				tinyip_ctrl.buf_free(pkt);
	}

	return ret;
}

int tinyip_ip_output(struct tinyip_buf *pkt, uint32_t src_ip, uint32_t dst_ip, uint8_t protocol)
{
	struct iphdr *ip;
	uint16_t len;
	static uint16_t ip_id = 0;
	char *dstmac;

	/* adjust pkt fields */
	len = pkt->length;
	pkt->sp --;
	pkt->length += sizeof(struct iphdr);
	pkt->stacks[pkt->sp] = pkt->stacks[pkt->sp+1] - sizeof(struct iphdr);

	ip = (struct iphdr *)(pkt->stacks[pkt->sp]);
	ip->version = IPVERSION;
	ip->ihl = 0x05;
	ip->tos = 0;
	ip->tot_len = tinyip_htons(len);
	ip->id = tinyip_htons(++ip_id);
	ip->frag_off = 0;
	ip->ttl = 255;
	ip->protocol = protocol;
	ip->saddr = tinyip_htonl(src_ip);
	ip->daddr = tinyip_htonl(dst_ip);
	ip->check = 0;

	ip->check = tinyip_chksum((void *)ip, sizeof(struct iphdr));

	if((dstmac = tinyip_arp_query(dst_ip, pkt)))
	{
		tinyip_ethernet_output(pkt, tinyip_ctrl.macaddr, dstmac, tinyip_htons(ETH_P_IP));
	}

	return 0;
}
