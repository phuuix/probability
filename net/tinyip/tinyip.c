/*
 * tinyip.c
 * In tinyip, we don't use any extern functions (including libc functions), 
 * because users may step into this functions when debug with gdb.
 * Defines some common functions.
 */

#include "tinyip.h"

struct tinyip_control tinyip_ctrl;


/********************************************************************
 * libc functions
 ********************************************************************/

unsigned short tinyip_htons(unsigned short hostshort) {
#if __BYTE_ORDER==__LITTLE_ENDIAN
  return ((hostshort>>8)&0xff) | (hostshort<<8);
#else
  return hostshort;
#endif
}

unsigned long tinyip_htonl(unsigned long hostlong) {
#if __BYTE_ORDER==__LITTLE_ENDIAN
  return (hostlong>>24) | ((hostlong&0xff0000)>>8) |
	  ((hostlong&0xff00)<<8) | (hostlong<<24);
#else
  return hostlong;
#endif
}


void *tinyip_memcpy(void *dest, const void *src, size_t n)
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


/********************************************************************
 * checksum functions
 ********************************************************************/

/*
 * checksum fixup
 * Taken directly from RFC 1631
 */
void tinyip_checksum_fixup (uint8_t *bptr_checksum, uint8_t *bptr_old_data,
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


static uint32_t chksum(void *dataptr, int len)
{
  uint32_t acc;
    
  for(acc = 0; len > 1; len -= 2) {
    acc += *((uint16_t *)dataptr)++;
  }

  /* add up any odd byte */
  if(len == 1) {
    acc += tinyip_htons((uint16_t)((*(uint8_t *)dataptr) & 0xff) << 8);
//    DEBUGF(INET_DEBUG, ("inet: chksum: odd byte %d\n", *(u8_t *)dataptr));
  }

  return acc;
}

uint16_t tinyip_chksum(void *dataptr, uint16_t len)
{
  uint32_t acc;

  acc = chksum(dataptr, len);
  while(acc >> 16) {
    acc = (acc & 0xffff) + (acc >> 16);
  }    
  return ~(acc & 0xffff);
}


/********************************************************************
 * interface functions
 ********************************************************************/
static int default_hw_send_handle(char *buf, int len)
{
	/* do nothing */
	return 0;
}

void tinyip_register_hw_send_handle(int (*hw_send_handle)())
{
	if(hw_send_handle)
		tinyip_ctrl.hw_send = hw_send_handle;
}

static void default_buf_free_handle(char *buf)
{
	/* do nothing */
}

void tinyip_register_buf_free_handle(void (*buf_free_handle)())
{
	if(buf_free_handle)
		tinyip_ctrl.buf_free = buf_free_handle;
}

void tinyip_init()
{
	tinyip_register_hw_send_handle(default_hw_send_handle);
	tinyip_register_buf_free_handle(default_buf_free_handle);

	tinyip_ethernet_init();
	tinyip_arp_init();
	tinyip_ip_init();
	#ifdef TINYIP_INCLUDE_ICMP
	tinyip_icmp_init();
	#endif
	tinyip_udp_init();
	tftp_init();
}



/*
 * set net interface's ip address
 *   ip: ip address
 */
void tinyip_ipaddr_set(uint32_t ip, uint32_t mask)
{
	tinyip_ctrl.ipaddr = ip;
	tinyip_ctrl.ipmask = mask;
}

/* 
 * get net interface's ip address
 *   return: ip address
 */
uint32_t tinyip_ipaddr_get()
{
	return tinyip_ctrl.ipaddr;
}

/*
 * get net interface's mac address
 */
char *tinyip_macaddr_get()
{
	return tinyip_ctrl.macaddr;
}

