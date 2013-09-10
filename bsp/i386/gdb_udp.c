/*
 * gdb_udp.c
 * gdb interface for udp
 * Don't use other functions that not in this file or xxx_stub.c
 */
#include <config.h>
#include <stdio.h>
#include <bsp.h>
#include <net/in.h>
#include <net/ip.h>
#include <net/icmp.h>
#include <net/udp.h>
#include <net/netintf.h>
#include <net/if_arp.h>
#include <net/if_ether.h>

#define BUFMAX 400

static struct ifnet *ifgdb;

static const char hexchars[]="0123456789abcdef";

static char gdb_udp_buffer[512];
static uint32_t ipaddress;
static char macaddress[6];

extern int hex(char ch);
extern size_t gdb_strlen(const char *s);

static void udp_pharse_packet(char *buffer, char *data, int len);
static void udp_get_packet(char *buffer);
static void udp_put_packet(char *buffer);

static unsigned short ghtons(unsigned short hostshort);
static unsigned long ghtonl(unsigned long hostlong);
static void *gmemcpy(void *dest, const void *src, size_t n);
static void checksum_fixup (uint8_t *bptr_checksum, uint8_t *bptr_old_data,
		uint16_t old_data_length, uint8_t *bptr_new_data, uint16_t new_data_length);

static unsigned short ghtons(unsigned short hostshort) {
#if __BYTE_ORDER==__LITTLE_ENDIAN
  return ((hostshort>>8)&0xff) | (hostshort<<8);
#else
  return hostshort;
#endif
}

static unsigned long ghtonl(unsigned long hostlong) {
#if __BYTE_ORDER==__LITTLE_ENDIAN
  return (hostlong>>24) | ((hostlong&0xff0000)>>8) |
	  ((hostlong&0xff00)<<8) | (hostlong<<24);
#else
  return hostlong;
#endif
}

#define gntohs ghtons
#define gntohl ghtonl

static void *gmemcpy(void *dest, const void *src, size_t n)
{
	char *r, *d, *s;
	
	if(dest==NULL || src==NULL)
		return NULL;
		
	r = (char *)dest;
	d = (char *)dest;
	s = (char *)src;
	
	while(n--)
		*d++ = *s++;
		
	return r;
}


/*
 * checksum fixup
 * Taken directly from RFC 1631
 */
static void checksum_fixup (uint8_t *bptr_checksum, uint8_t *bptr_old_data,
		uint16_t old_data_length, uint8_t *bptr_new_data, uint16_t new_data_length)
{
	uint32_t working_checksum;
	uint32_t old_data_word;
	uint32_t new_data_word;

	working_checksum = (uint32_t) ((bptr_checksum[0] * 256) + bptr_checksum[1]);

	working_checksum = (~working_checksum & 0x0000FFFF);

	while (old_data_length > 0x0000)
		{
		if (old_data_length == 0x00000001)
			{
			old_data_word = (uint32_t) ((bptr_old_data[0] * 256) + bptr_old_data[1]);

			working_checksum = working_checksum - (old_data_word & 0x0000FF00);

			if ((long) working_checksum <= 0x00000000L)
				{
				--working_checksum;

				working_checksum = working_checksum & 0x0000FFFF;
				}

			break;
			}
		else
			{
			old_data_word = (uint32_t) ((bptr_old_data[0] * 256) + bptr_old_data[1]);

			bptr_old_data = bptr_old_data + 2;

			working_checksum = working_checksum - (old_data_word & 0x0000FFFF);

			if ((long) working_checksum <= 0x00000000L)
				{
				--working_checksum;

				working_checksum = working_checksum & 0x0000FFFF;
				}

			old_data_length = (uint16_t) (old_data_length - 2);
			}
		}

	while (new_data_length > 0x0000)
		{
		if (new_data_length == 0x00000001)
			{
			new_data_word = (uint32_t) ((bptr_new_data[0] * 256) + bptr_new_data[1]);

			working_checksum = working_checksum + (new_data_word & 0x0000FF00);

			if (working_checksum & 0x00010000)
				{
				++working_checksum;

				working_checksum = working_checksum & 0x0000FFFF;
				}

			break;
			}
		else
			{
			new_data_word = (uint32_t) ((bptr_new_data[0] * 256) + bptr_new_data[1]);

			bptr_new_data = bptr_new_data + 2;

			working_checksum = working_checksum + (new_data_word & 0x0000FFFF);

			if (working_checksum & 0x00010000)
				{
				++working_checksum;

				working_checksum = working_checksum & 0x0000FFFF;
				}

			new_data_length = (uint16_t) (new_data_length - 2);
			}
		}

	working_checksum = ~working_checksum;

	bptr_checksum[0] = (uint8_t) (working_checksum/256);
	bptr_checksum[1] = (uint8_t) (working_checksum & 0x000000FF);
}


static void udp_pharse_packet(char *buffer, char *data, int len)
{
	unsigned char checksum;
	unsigned char xmitcsum;
	int  i;
	int  count;
	char ch;

	for(i=0; i<len; i++)
	{
		/* find start character '$' */
		if((data[i] & 0x7f ) == '$')
			break;
	}

	checksum = 0;
	xmitcsum = -1;

	count = 0;

	while(i<len)
	{
		i ++;
		ch = data[i] & 0x7f;
		if(ch == '#')
		{
			xmitcsum = hex(data[i+1] & 0x7f) << 4;
			xmitcsum += hex(data[i+2] & 0x7f);
			break;
		}
		checksum = checksum + ch;
		buffer[count] = ch;
		count ++;
	}
	buffer[count] = 0;

	if(checksum == xmitcsum)
	{	/* successful transfer */
		char tmpchar[4];

		tmpchar[0] = '+';
		tmpchar[1] = 0;
		if(buffer[2] == ':'){
			/* if a sequence char is present, reply the sequence ID */
			tmpchar[1] = buffer[0];
			tmpchar[2] = buffer[1];
			tmpchar[3] = 0;
			for(i=3; i<count; i++) buffer[i-3] = buffer[i];
		}
		
		udp_put_packet(tmpchar);
	}
	else
	{	/* failed checksum */
		udp_put_packet("-");
		buffer[0] = '-';
	}
}


static void udp_get_packet(char *buffer)
{
	int ret;
	struct iphdr *ip;
	struct icmphdr *icmp;
	struct udphdr *udp;
	struct ethhdr *eth;
	struct arphdr *arp;
	char *data;
	char *buf = gdb_udp_buffer;

	while(1)
	{
		ret = ifgdb->if_poll_recv(0, buf, 512);
		if(ret > 0)
		{
			eth = (struct ethhdr *)buf;
			if(gntohs(eth->h_proto) == ETH_P_IP)
			{
				ip = (struct iphdr *)(buf + 14);

				if(gntohl(ip->daddr) != ipaddress)
				{
//					kprintf("ipaddress(0x%x)=0x%x ip->daddr=0x%x\n", &ipaddress, ipaddress, ip->daddr);
					continue;
				}

				if(ip->protocol == IPPROTO_ICMP)	/* ICMP */
				{
					icmp = (struct icmphdr *)(ip + 1);
					if(icmp->type == ICMP_ECHO)	/* echo request */
					{
						uint8_t tmpchar;

						/* adjust ethernet header */
						gmemcpy(buf, buf+6, 6);
						gmemcpy(buf+6, macaddress, 6);

						/* adjust ip header */
						ip->daddr = ip->saddr;
						ip->saddr = ghtonl(ipaddress);

						/* adjust icmp packet */
						tmpchar = ICMP_ECHOREPLY;
						checksum_fixup((uint8_t *)&icmp->checksum, (uint8_t *)&icmp->type, 1, (uint8_t *)&tmpchar, 1);
						icmp->type = ICMP_ECHOREPLY;

						/* send packet */
						if(ifgdb->if_poll_send(0, buf, ret) == -1)
							kprintf("GDB stub send packet failed.\n");
					}
				}
				else if(ip->protocol == IPPROTO_UDP)	/* UDP */
				{
					uint16_t tmpshort, len;

					udp = (struct udphdr *)(ip + 1);
					data = (char *)(udp + 1);
					len = gntohs(udp->len) - sizeof(struct udphdr);

					if(len >= BUFMAX)
						continue;

					/* adjust ethernet header */
					gmemcpy(buf, buf+6, 6);
					gmemcpy(buf+6, macaddress, 6);

					/* adjust ip header */
					ip->daddr = ip->saddr;
					ip->saddr = ghtonl(ipaddress);

					/* adjust udp header */
					tmpshort = udp->source;
					udp->source = udp->dest;
					udp->dest = tmpshort;

					if(data[0] == '$')
					{
						udp_pharse_packet(buffer, data, len);
						break;
					}
					else if(data[0] == '-')
						kprintf("GDB receive packet: -\n");
				}
			}
			else if(gntohs(eth->h_proto) == ETH_P_ARP)
			{
				arp = (struct arphdr *)(buf + 14);
				if(gntohs(arp->ar_op) == ARPOP_REQUEST)		/* request */
				{
					uint32_t arpip, sendip;
					int i;

					data = (char *)(arp + 1);
					sendip = *(uint32_t *)(data + 6);
					arpip = *(uint32_t *)(data + 16);
					if(gntohl(arpip) == ipaddress)
					{
						/* adjust ethernet header */
						gmemcpy(buf, buf+6, 6);
						gmemcpy(buf+6, macaddress, 6);

						/* adjust arp packet */
						arp->ar_op = ghtons(ARPOP_REPLY);
						*(uint32_t *)(data + 6) = arpip;
						*(uint32_t *)(data + 16) = sendip;
						for(i=0; i<6; i++)
						{
							/* fill dst mac address */
							*(data + 10 + i) = *(data + i);
							/* fill src mac address */
							*(data + i) = macaddress[i];
						}

						/* send packet */
						if(ifgdb->if_poll_send(0, buf, ret) == -1)
							kprintf("GDB send packet failed.\n");
					}
				}
			}
		}
	}
}

static void udp_put_packet(char *buffer)
{
	struct iphdr *ip;
	struct udphdr *udp;
	char *data;
	int buflen;

	uint16_t ip_len;

	buflen = gdb_strlen(buffer);

	ip = (struct iphdr *)(gdb_udp_buffer + 14);
	udp = (struct udphdr *)(ip + 1);
	data = (char *)(udp + 1);

	if(buffer[0] == '+' || buffer[0] == '-')
	{
		/* send packet directly */
		gmemcpy(data, buffer, buflen);
	}
	else
	{
		unsigned char checksum = 0;
		int count;

		data[0] = '$';
		for(count=0; count<buflen; count++)
		{
			data[count+1] = buffer[count];
			checksum += buffer[count];
		}
		data[count+1] = '#';
		data[count+2] = hexchars[checksum >> 4];
		data[count+3] = hexchars[checksum % 16];

		buflen += 4;
	}

	udp->len = ghtons(buflen + sizeof(struct udphdr));
	udp->check = ghtons(0);

	ip_len = ghtons(buflen + 20 + 8);
	checksum_fixup((uint8_t *)&ip->check, (uint8_t *)&ip->tot_len, sizeof(uint16_t),
		(uint8_t *)&ip_len, sizeof(uint16_t));
	ip->tot_len = ip_len;

	if(ifgdb->if_poll_send(0, gdb_udp_buffer, buflen + 14 + 20 + 8) == -1)
		kprintf("GDB stub send packet failed.\n");
}


extern void (*gdb_get_packet)(char *);
extern void (*gdb_put_packet)(char *);
extern struct ifnet c509_ifnet;
extern struct ifnet rtl8139_ifnet;

/* init udp for gdb */
void gdb_init_udp()
{
	/* ifgdb = &c509_ifnet; */
	/* ifgdb = &rtl8139_ifnet; */
	ipaddress = GDB_NET_IPADDRESS;

	ifgdb->if_ioctl(0, NIOCTL_GADDR, macaddress);

	gdb_get_packet = udp_get_packet;
	gdb_put_packet = udp_put_packet;
}

/* set poll mode to net interface */
void gdb_udp_poll_start()
{
	ifgdb->if_ioctl(0, NIOCTL_POLLSTART, NULL);
}

/* unset poll mode to net interface */
void gdb_udp_poll_stop()
{
	ifgdb->if_ioctl(0, NIOCTL_POLLSTOP, NULL);
}

/*
 * set net interface's ip address
 *   ip: ip address
 */
void gdb_udp_ipaddr_set(uint32_t ip)
{
	ipaddress = ip;
}

/* 
 * get net interface's ip address
 *   return: ip address
 */
uint32_t gdb_udp_ipaddr_get()
{
	return ipaddress;
}

