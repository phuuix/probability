/*
 * tinyip.h
 */
#ifndef __D_TINYIP_H__
#define __D_TINYIP_H__

#include <sys/types.h>

#define TINYIP_INCLUDE_ICMP

/***************************************************************
 * defination for buffer management
 ***************************************************************/
#define STACKS_NUMBER 4

struct tinyip_buf
{
	char *stacks[STACKS_NUMBER];
	short sp;
	short length;
	char *buffer;
	uint32_t flags;
};

#define TINYIP_INPUT_OFFSET 64
#define TINYIP_OUTPUT_OFFSET 128

/* flags */
#define TINYIP_FLAG_OUTSIDE 0x01	/* pkt come from outside */

/***************************************************************
 * defination for tinyip control struct
 ***************************************************************/

#define ARP_ENTRY_INVALID	0
#define ARP_ENTRY_VALID		1

struct tinyip_arp_table
{
	char macaddr[6];
	uint16_t flag;
	uint32_t ipaddr;
	int timeout;
	struct tinyip_buf *packet;
};

/* control struct for one interface */
struct tinyip_control
{
	uint32_t ipaddr;							/* interface's ip address */
	uint32_t ipmask;							/* interface's ip mask */
	char macaddr[6];						/* interface's mac address */
	struct tinyip_arp_table arp_table;		/* interface's arp table (only support one entry now) */
	int (*hw_recv)();						/* interface's hardware receive function */
	int (*hw_send)();						/* interface's hardware send function */
	void (*buf_free)();						/* buffer free handle */
};

extern struct tinyip_control tinyip_ctrl;


unsigned short tinyip_htons(unsigned short hostshort);
unsigned long tinyip_htonl(unsigned long hostlong);
#define tinyip_ntohs tinyip_htons
#define tinyip_ntohl tinyip_htonl

void *tinyip_memcpy(void *dest, const void *src, size_t n);
uint16_t tinyip_chksum(void *dataptr, uint16_t len);
void tinyip_checksum_fixup (uint8_t *bptr_checksum, uint8_t *bptr_old_data,
		uint16_t old_data_length, uint8_t *bptr_new_data, uint16_t new_data_length);

void tinyip_ipaddr_set(uint32_t ip, uint32_t mask);
uint32_t tinyip_ipaddr_get();
char *tinyip_macaddr_get();

void tinyip_register_hw_send_handle(int (*hw_send_handle)());
void tinyip_register_buf_free_handle(void (*buf_free_handle)());
void tinyip_init();



void tinyip_arp_init();
int tinyip_arp_input(struct tinyip_buf *pkt);
int tinyip_arp_output(struct tinyip_buf *pkt);
int tinyip_arp_send_request(uint32_t targetip);
char *tinyip_arp_query(uint32_t targetip, struct tinyip_buf *pkt);

void tinyip_ethernet_init();
int tinyip_ethernet_input(char *ethframe, int length);
int tinyip_ethernet_output(struct tinyip_buf *pkt, char *srcaddr, char *dstaddr, uint16_t frametype);

void tinyip_icmp_init();
int tinyip_icmp_input(struct tinyip_buf *pkt);
int tinyip_icmp_output(struct tinyip_buf *pkt);

void tinyip_ip_init();
int tinyip_ip_input(struct tinyip_buf *pkt);
int tinyip_ip_output(struct tinyip_buf *pkt, uint32_t src_ip, uint32_t dst_ip, uint8_t protocol);

int tftp_send_rrq(char *buf, uint32_t servip, uint16_t servport, uint16_t clientport, char *filename);
int tftp_send_ack(char *buf, uint32_t servip, uint16_t servport, uint16_t clientport, uint16_t blockno);
int tftp_handler(uint32_t from_ip, uint16_t from_port, uint16_t to_port, uint8_t *pkt, int len);
void tftp_init();
void tftp_start();

void tinyip_udp_init();
int tinyip_udp_input(struct tinyip_buf *pkt);
int tinyip_udp_output(char *buf, int len, uint32_t src_ip, uint32_t dst_ip, uint16_t src_port, uint16_t dst_port);
int tinyip_register_udp_handle(uint16_t port, int (*udp_handle)());
int tinyip_unregister_udp_handle(uint16_t port);

#endif /* __D_TINYIP_H__ */
