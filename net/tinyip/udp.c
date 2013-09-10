/*
 * udp.c
 */
#include <sys/types.h>
#include <net/udp.h>
#include <net/in.h>
#include <net/ip.h>

#include "tinyip.h"

#define MAX_UDP_PORTS 2

struct tinyip_udp_stub
{
	uint16_t valid;
	uint16_t port;
	int (*handle)();
};

static struct tinyip_udp_stub udp_stub[MAX_UDP_PORTS];


void tinyip_udp_init()
{
	int i;

	for(i=0; i<MAX_UDP_PORTS; i++)
		udp_stub[i].valid = 0;
}


int tinyip_udp_input(struct tinyip_buf *pkt)
{
	struct udphdr *udp;
	int i, ret = 0;
	uint16_t dst_port, src_port;
	uint32_t src_ip;

	udp = (struct udphdr *)(pkt->stacks[pkt->sp]);
	dst_port = tinyip_ntohs(udp->dest);

	pkt->sp ++;
	pkt->stacks[pkt->sp] = (char *)(udp + 1);
	pkt->length -= sizeof(struct udphdr);

	for(i=0; i<MAX_UDP_PORTS; i++)
	{
		if(udp_stub[i].valid && udp_stub[i].port == dst_port && udp_stub[i].handle)
			break;
	}

	if(i != MAX_UDP_PORTS)
	{
		struct iphdr *ip;

		ip = (struct iphdr *)(pkt->stacks[pkt->sp-1]);
		src_ip = tinyip_ntohl(ip->saddr);
		src_port = tinyip_ntohs(udp->source);
		ret = udp_stub[i].handle(src_ip, dst_port, src_port, pkt->stacks[pkt->sp], pkt->length);
	}

	if(pkt->flags & TINYIP_FLAG_OUTSIDE)
		tinyip_ctrl.buf_free(pkt);

	return ret;
}


int tinyip_udp_output(char *buf, int len, uint32_t src_ip, uint32_t dst_ip, uint16_t src_port, uint16_t dst_port)
{
	struct tinyip_buf *pkt;
	struct udphdr *udp;

	pkt = (struct tinyip_buf *)(buf - TINYIP_OUTPUT_OFFSET);
	pkt->sp = 3;
	pkt->stacks[pkt->sp] = buf - sizeof(struct udphdr);
	pkt->length = len + sizeof(struct udphdr);
	pkt->buffer = (char *)pkt;
	pkt->flags = TINYIP_FLAG_OUTSIDE;

	udp = (struct udphdr *)(pkt->stacks[pkt->sp]);
	udp->source = tinyip_htons(src_port);
	udp->dest = tinyip_htons(dst_port);
	udp->len = len;
	udp->check = 0;

	if(src_ip == 0)
		src_ip = tinyip_ctrl.ipaddr;

	return tinyip_ip_output(pkt, src_ip, dst_ip, IPPROTO_UDP);
}

/* register a udp handle */
int tinyip_register_udp_handle(uint16_t port, int (*udp_handle)())
{
	int i;

	for(i=0; i<MAX_UDP_PORTS; i++)
	{
		if(udp_stub[i].valid == 0)
		{
			udp_stub[i].valid = 1;
			udp_stub[i].port = port;
			udp_stub[i].handle = udp_handle;
			return i;
		}
	}

	return 0;
}

/* unregister a udp handle */
int tinyip_unregister_udp_handle(uint16_t port)
{
	int i;

	for(i=0; i<MAX_UDP_PORTS; i++)
	{
		if(udp_stub[i].valid == 1 && udp_stub[i].port == port)
		{
			udp_stub[i].valid = 0;
			udp_stub[i].port = 0;
			udp_stub[i].handle = NULL;
			return i;
		}
	}

	return 0;
}
