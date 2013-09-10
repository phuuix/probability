/*
 * tftp.c
 * A TFTP client
 */

#include <bsp.h>

#include "tinyip.h"

/***************************************************************
 * defination for TFTP protocol
 ***************************************************************/
#define TFTP_SERVER_PORT 69

/* TFTP operations */
#define TFTP_RRQ	1
#define TFTP_WRQ	2
#define TFTP_DATA	3
#define TFTP_ACK	4
#define TFTP_ERROR	5

/* TFTP states */
#define TFTP_STATE_RRQ			1
#define TFTP_STATE_DATA			2
#define TFTP_STATE_TOO_LARGE	3
#define TFTP_STATE_BAD_MAGIC	4


static uint16_t tftp_lastblock;
static char *tftp_filename;
static uint32_t tftp_servip;
static uint16_t tftp_localport;

int tftp_send_rrq(char *buf, uint32_t servip, uint16_t servport, uint16_t clientport, char *filename)
{
	char * pkt;
	int len = 0;

	pkt = buf;
	*((uint16_t *)pkt)++ = tinyip_htons(TFTP_RRQ);
	while(*filename)
	{
		len ++;
		*pkt++ = *filename++;
	}
	*pkt++ = '\0';
	*pkt++ = 'o';
	*pkt++ = 'c';
	*pkt++ = 't';
	*pkt++ = 'e';
	*pkt++ = 't';
	*pkt++ = '\0';

	return tinyip_udp_output(buf, pkt-buf, tinyip_ctrl.ipaddr, servip, clientport, servport);
}

int tftp_send_ack(char *buf, uint32_t servip, uint16_t servport, uint16_t clientport, uint16_t blockno)
{
	char * pkt;

	pkt = buf;
	*((uint16_t *)pkt)++ = tinyip_htons(TFTP_ACK);
	*((uint16_t *)pkt)++ = tinyip_htons(blockno);

	return tinyip_udp_output(buf, pkt-buf, tinyip_ctrl.ipaddr, servip, clientport, servport);
}

int tftp_handler(uint32_t from_ip, uint16_t from_port, uint16_t to_port, uint8_t *pkt, int len)
{
	int ret = 0;
	uint16_t op, block_no;

	op = *((uint16_t *)pkt)++;
	op = tinyip_ntohs(op);
	len -= 2;

	switch (op) {

	case TFTP_RRQ:
	case TFTP_WRQ:
	case TFTP_ACK:
		break;

	case TFTP_DATA:
		if (len < 2)
			break;
		len -= 2;

		block_no = tinyip_ntohs(*(uint16_t *)pkt);
		if(block_no != tftp_lastblock + 1)
			break;

		if (((block_no - 1) % 10) == 0) putc ('#');

		/* save data */

		/* send ack */
		tftp_send_ack(pkt, tftp_servip, TFTP_SERVER_PORT, tftp_localport, block_no);

		if(len != 512)
		{
			ret = -1;		/* receive finished */
			kprintf("\nTFTP done\n");
		}
		break;
	
	case TFTP_ERROR:
		kprintf ("\nTFTP error\n");
		ret = -1;
		break;
	}

	return ret;
}

void tftp_init()
{
	tftp_lastblock = 0;
	tftp_filename = "dooloo.bin";
	tftp_localport = 2222;
}

void tftp_start()
{
	char buf[160];

	tftp_send_rrq(buf + TINYIP_OUTPUT_OFFSET, tftp_servip, TFTP_SERVER_PORT, tftp_localport, tftp_filename);
}
