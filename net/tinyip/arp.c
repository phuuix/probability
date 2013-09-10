/*
 * arp.c
 */

#include <bsp.h>
#include <net/if_arp.h>
#include <net/if_ether.h>

#include "tinyip.h"

void tinyip_arp_init()
{
	/* init arp table */
	tinyip_ctrl.arp_table.ipaddr = 0;
	tinyip_ctrl.arp_table.packet = NULL;
}

int tinyip_arp_input(struct tinyip_buf *pkt)
{
	struct arphdr *arp;
	char *data, *dstmac;
	uint32_t arpip, sendip;

	arp = (struct arphdr *)(pkt->stacks[pkt->sp]);
	dstmac = pkt->stacks[pkt->sp-1] + 6;

	switch(tinyip_ntohs(arp->ar_op))
	{
		case ARPOP_REQUEST:		/* request */
			data = (char *)(arp + 1);
			sendip = *(uint32_t *)(data + 6);
			arpip = *(uint32_t *)(data + 16);
			if(tinyip_ntohl(arpip) == tinyip_ctrl.ipaddr)
			{
				/* adjust arp packet */
				arp->ar_op = tinyip_htons(ARPOP_REPLY);
				*(uint32_t *)(data + 6) = arpip;
				*(uint32_t *)(data + 16) = sendip;
				tinyip_memcpy(data+10, data, 6);				/* target's mac address */
				tinyip_memcpy(data, tinyip_ctrl.macaddr, 6);	/* sender's mac address */

				/* update arp table entry */
				if(tinyip_ctrl.arp_table.packet == NULL)
				{
					tinyip_memcpy(tinyip_ctrl.arp_table.macaddr, data+10, 6);
					tinyip_ctrl.arp_table.ipaddr = tinyip_ntohl(sendip);
				}

				/* send packet */
				tinyip_ethernet_output(pkt, tinyip_ctrl.macaddr, dstmac, tinyip_htons(ETH_P_ARP));
			}

			break;

		case ARPOP_REPLY:		/* reply */
			data = (char *)(arp + 1);
			sendip = *(uint32_t *)(data + 6);
			
			if(tinyip_ntohl(sendip) == tinyip_ctrl.arp_table.ipaddr && tinyip_ctrl.arp_table.packet)
			{
				/* update arp table entry */
				tinyip_memcpy(tinyip_ctrl.arp_table.macaddr, data, 6);

				/* send pending packet */
				tinyip_ethernet_output(tinyip_ctrl.arp_table.packet, tinyip_ctrl.macaddr, data, tinyip_htons(ETH_P_IP));

				tinyip_ctrl.arp_table.packet = NULL;
			}
			
			if(pkt->flags & TINYIP_FLAG_OUTSIDE)
				tinyip_ctrl.buf_free(pkt);

			break;
		
		default:
			if(pkt->flags & TINYIP_FLAG_OUTSIDE)
				tinyip_ctrl.buf_free(pkt);
	}

	return 0;
}

int tinyip_arp_output(struct tinyip_buf *pkt)
{
	return 0;
}

/* send arp request for target ip's mac address */
int tinyip_arp_send_request(uint32_t targetip)
{
	char arp_buf[128];

	struct arphdr *arp;
	char *data;
	uint32_t sendip;
	struct tinyip_buf *pkt;
	
	pkt = (struct tinyip_buf *)arp_buf;
	pkt->flags = 0;
	pkt->buffer = arp_buf;

	pkt->sp = 1;
	pkt->stacks[1] = pkt->buffer + sizeof(struct tinyip_buf) + sizeof(struct ethhdr);
	pkt->length = sizeof(struct arphdr) + 20;

	arp = (struct arphdr *)(pkt->stacks[pkt->sp]);
	arp->ar_hrd = tinyip_htons(ARPHRD_ETHER);
	arp->ar_pro = tinyip_htons(ETH_P_IP);
	arp->ar_hln = 6;
	arp->ar_pln = 4;
	arp->ar_op = tinyip_htons(ARPOP_REQUEST);

	data = (char *)(arp + 1);
	tinyip_memcpy(data, tinyip_ctrl.macaddr, 6);
	sendip = tinyip_htonl(tinyip_ctrl.ipaddr);
	tinyip_memcpy(data+6, &sendip, 4);
	tinyip_memcpy(data+10, "\0xff\0xff\0xff\0xff\0xff\0xff", 6);
	targetip = tinyip_htonl(targetip);
	tinyip_memcpy(data+16, &targetip, 4);

	tinyip_ethernet_output(pkt, tinyip_ctrl.macaddr, "\0xff\0xff\0xff\0xff\0xff\0xff", tinyip_htons(ETH_P_ARP));

	return 0;
}

char *tinyip_arp_query(uint32_t targetip, struct tinyip_buf *pkt)
{
	if(targetip == tinyip_ctrl.arp_table.ipaddr && tinyip_ctrl.arp_table.packet)
	{
		return tinyip_ctrl.arp_table.macaddr;
	}
	else
	{
		tinyip_arp_send_request(targetip);
		tinyip_ctrl.arp_table.ipaddr = targetip;
		tinyip_ctrl.arp_table.packet = pkt;
	}

	return NULL;
}
