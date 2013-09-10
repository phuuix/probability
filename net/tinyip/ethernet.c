/*
 * ethernet.c
 */
#include <bsp.h>
#include <net/if_arp.h>
#include <net/if_ether.h>

#include "tinyip.h"

void tinyip_ethernet_init()
{
	/* get our mac address */
}

int tinyip_ethernet_input(char *ethframe, int length)
{
	int ret = 0;
	struct ethhdr *eth;
	struct tinyip_buf *pkt;

	pkt = (struct tinyip_buf *)(ethframe - TINYIP_INPUT_OFFSET);
	pkt->flags = TINYIP_FLAG_OUTSIDE;
	pkt->sp = 0;
	pkt->buffer = (char *)pkt;
	pkt->length = length;
	pkt->stacks[0] = ethframe;

	eth = (struct ethhdr *)(pkt->stacks[pkt->sp]);
	pkt->sp++;
	pkt->stacks[pkt->sp] = (char *)(eth + 1);
	pkt->length -= sizeof(struct ethhdr);

	switch(tinyip_ntohs(eth->h_proto))
	{
		case ETH_P_IP:
			ret = tinyip_ip_input(pkt);
			break;
		case ETH_P_ARP:
			ret = tinyip_arp_input(pkt);
			break;
		default:
			if(pkt->flags & TINYIP_FLAG_OUTSIDE)
				tinyip_ctrl.buf_free(pkt);
	}

	return ret;
}

int tinyip_ethernet_output(struct tinyip_buf *pkt, char *srcaddr, char *dstaddr, uint16_t frametype)
{
	char *buf = pkt->stacks[pkt->sp];

	buf -= sizeof(struct ethhdr);
	pkt->length += sizeof(struct ethhdr);

	tinyip_memcpy(buf, dstaddr, 6);
	tinyip_memcpy(buf+6, srcaddr, 6);
	frametype = tinyip_htons(frametype);
	tinyip_memcpy(buf+12, &frametype, 2);

	tinyip_ctrl.hw_send(0, buf, pkt->length);
	if(pkt->flags & TINYIP_FLAG_OUTSIDE)
		tinyip_ctrl.buf_free(pkt);

	return 0;
}
