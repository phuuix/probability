/*
 * 3c509.c
 *	3Com 3C509 Ethernet card support.
 */
#include <string.h>
#include <stdlib.h>
#include <task.h>
#include <ipc.h>
#include <assert.h>
#include <lock.h>
#include <list.h>
#include <bsp.h>
#include <net/in.h>
#include <net/netintf.h>

#include "i386_hw.h"
#include "3c509.h"

typedef unsigned char u8_t;
typedef unsigned short u16_t;
typedef unsigned long u32_t;

#define C509_EVENT_INTERRUPT	0
#define C509_EVENT_SENDDATA		1

struct ifnet c509_ifnet = {
	c509_init, c509_cleanup, c509_open, c509_close, c509_ioctl, c509_recv, c509_send, c509_poll_recv, c509_poll_send
};
static struct c509_control c509_ctl;


#if 0
static void shrink(int x, int y, char c)
{
	static char attr = BLUE;

	attr ^= 0x01;
	putc_xy(x, y, attr, c);
}
#endif /* 0 */

/*
 * isr_3c509()
 */
static void isr_3c509(void *p)
{	

}


/*
 * c509_intr_task()
 */
//static void c509_intr_task(void *p) __attribute__ ((noreturn));
static void c509_intr_task(void *p)
{

}

/*
 * eeprom_read()
 *
 * We assume that we're in register window 0 on entry.
 */
static u16_t eeprom_read(u8_t addr)
{
	u32_t i;
	
	c509_write16(C509_W0_EEPROM_CMD, (u16_t)addr + 0x80);

	/*
	 * Pause for 162 us.
	 */
	for (i = 0; i < 500000; i++);

	return c509_read16(C509_W0_EEPROM_DATA);
}

/*
 * c509_hw_init()
 *	Initialize the 3C509 card.
 */
void c509_hw_init()
{
	int i;
	u16_t ids = 0x00ff;
	u8_t *p;

	/*
	 * Jump through hoops in the style described in the 3Com
	 * documentation - don't ask ... it's not worth it :-/
	 */
	c509_write8id(0x00);		/* select 0x110 as ID port */
	c509_write8id(0x00);		/* set IDS to init state */
	for (i = 0; i < 256; i++) {
	c509_write8id((u8_t)ids);
	ids <<= 1;
	ids = (ids & 0x0100) ? (ids ^ 0x00cf) : ids;
	}
	c509_write8id(0xd1);		/* set adapter register to 1 */

	/*
	 * Enable the board at address 0x0300 (as far as the ISA bus is
	 * concerned that is!).
	 */
	c509_write8id(0xe0 | ((C509_IO_PORT_BASE-0x200) >> 4));

	/*
	 * Get our MAC address.
	 */
	c509_write16(C509_CMD, CMD_SELECT_WINDOW | 0);
	for (i = 0; i < 3; i++) {
       	*((u16_t *)&c509_ctl.mac_addr[i << 1]) = htons(eeprom_read(i));
	}
	
	/*
	 * Enable the adaptor.
	 */
	c509_write16(C509_W0_CONFIG_CTRL, 0x0001);	/* ? no 10BASE-T transceiver and no VOC ? */

	/*
	 * Reset the transmitter and receiver and temporarily suspend all
	 * status flags.
	 */
	c509_write16(C509_CMD, CMD_TX_RESET);
	c509_write16(C509_CMD, CMD_RX_RESET);
	c509_write16(C509_CMD, CMD_SET_READ_ZERO_MASK | 0x0000);
		
	/*
	 * Set IRQ.
	 */
	c509_write16(C509_W0_RESOURCE_CFG, 0x0f00|(C509_IRQ<<12));
	
	/*
	 * Set our MAC address.
	 */
	c509_write16(C509_CMD, CMD_SELECT_WINDOW | 2);
	p = (u8_t *)c509_ctl.mac_addr;
	c509_write8(C509_W2_ADDR0, *p++);
	c509_write8(C509_W2_ADDR1, *p++);
	c509_write8(C509_W2_ADDR2, *p++);
	c509_write8(C509_W2_ADDR3, *p++);
	c509_write8(C509_W2_ADDR4, *p++);
	c509_write8(C509_W2_ADDR5, *p++);
	
	/*
	 * Set media type to twisted pair (link beat and jabber enable)
	 */
	c509_write16(C509_CMD, CMD_SELECT_WINDOW | 4);
	c509_write16(C509_W4_MEDIA_TYPE, c509_read16(C509_W4_MEDIA_TYPE) | 0x00c0);
	
	/*
	 * Clear down all of the statistics (by reading them).
	 */
	c509_write16(C509_CMD, CMD_STATS_DISABLE);
	c509_write16(C509_CMD, CMD_SELECT_WINDOW | 6);
	c509_read8(0x00);
	c509_read8(0x01);
	c509_read8(0x02);
	c509_read8(0x03);
	c509_read8(0x04);
	c509_read8(0x05);
	c509_read8(0x06);
	c509_read8(0x07);
	c509_read8(0x08);
	c509_read8(0x09);
	c509_read8(0x0a);
	c509_read8(0x0b);
	c509_read8(0x0c);
	c509_read8(0x0d);
	c509_write16(C509_CMD, CMD_STATS_ENABLE);

	/*
	 * Set to receive host address and broadcasts.
	 */
	c509_write16(C509_CMD, CMD_SELECT_WINDOW | 1);
    c509_write16(C509_CMD, CMD_SET_RX_FILTER | 0x0005);

	/*
	 * Enable the receiver and transmitter.
	 */
	c509_write16(C509_CMD, CMD_RX_ENABLE);
	c509_write16(C509_CMD, CMD_TX_ENABLE);

	/*
	 * Allow status bits through.
	 */
	c509_write16(C509_CMD, CMD_SET_READ_ZERO_MASK | 0x00ff);

	/*
	 * Acknowledge any pending interrupts and enable those that we want to
	 * see during operations.
	 */
	c509_write16(C509_CMD, CMD_ACK_INT | STAT_INT_LATCH | STAT_TX_AVAILABLE
        | STAT_RX_EARLY | STAT_INT_REQUESTED);
	c509_write16(C509_CMD, CMD_SET_INT_MASK | STAT_INT_LATCH | STAT_ADAPTER_FAILURE
		| STAT_TX_COMPLETE | STAT_RX_COMPLETE | STAT_UPDATE_STATS);
}


/* standard driver function: init() */
void c509_init()
{
	c509_hw_init();
}


/* standard driver function: cleanup() */
void c509_cleanup()
{
}


/* standard driver function: open() */
int c509_open(const char *pathname, int flags)
{
	return 1;
}


/* standard driver function: close() */
int c509_close(int d)
{
	return 0;
}


/* standard driver function: ioctl() */
int c509_ioctl(int d, int request, void* p)
{
	int ret=0;

	switch(request)
	{
		case NIOCTL_POLLSTART:
			/* clean up transmit queue */
			
			/* mask interrupts */
			c509_write16(C509_CMD, CMD_SET_INT_MASK | 0);

			c509_ctl.flags |= C509_FLAG_POLL;
			break;

		case NIOCTL_POLLSTOP:
			/* unmask interrupts */
			c509_write16(C509_CMD, CMD_SET_INT_MASK | STAT_INT_LATCH | STAT_ADAPTER_FAILURE
				| STAT_TX_COMPLETE | STAT_RX_COMPLETE | STAT_UPDATE_STATS);

			c509_ctl.flags &= ~C509_FLAG_POLL;
			break;

		case NIOCTL_GADDR:
			/* get mac address */
			if(p)
				memcpy(p, c509_ctl.mac_addr, 6);
			else
				ret = -1;
			break;

		default :
			break;
	}

	return ret;
}


/* standard driver function: recv() */
int c509_recv(int d, void *buf, size_t count)
{
	return -1;
}


/* standard driver function: send() */
int c509_send(int d, const void *buf, size_t count)
{
	return 0;
}


/* standard driver function: poll_recv() */
int c509_poll_recv(int d, char *buf, size_t count)
{
	u16_t stat;
	u16_t pktsz=0;
	
	/* if driver isn't under poll mode */
	if(! c509_ctl.flags&C509_FLAG_POLL)
		return -1;
		
	/*
	 * Did we have a successful receive?
	 */
	stat = c509_read16(C509_W1_RX_STATUS);
	if(stat & 0x8000)
	{	/* incomplete */
		return -1;
	}
//	kprintf("3C509 rx status: %x\n", stat);
//	if (!stat) {
//		return -1;
//	}

	if (!(stat & 0x4000)) {
		u16_t words;
		
		pktsz = ((stat & 0x07ff) + 1) & 0xfffe;

		if(pktsz > count)
		{
			/* buffer too small */
			/*
			 * Discard the packet and wait for the discard operation to complete.
			 */
			c509_write16(C509_CMD, CMD_RX_DISCARD);
			while (c509_read16(C509_STATUS) & 0x1000);
			return -1;
		}

		/*
		 * Read the packet.
		 */
		words = pktsz >> 1;
		while (words) {
			*(u16_t *)buf = c509_read16(C509_W1_RX_DATA);
			buf += sizeof(u16_t);
			words--;
		}
	}

	/*
	 * Discard the packet and wait for the discard operation to complete.
	 */
	c509_write16(C509_CMD, CMD_RX_DISCARD);
	while (c509_read16(C509_STATUS) & 0x1000);
	
	return pktsz;
}


/* standard driver function: poll_send() */
int c509_poll_send(int d, const char *buf, size_t count)
{
	u16_t words;
	u16_t st;

	/* if driver isn't under poll mode */
	if(! c509_ctl.flags&C509_FLAG_POLL)
		return -1;
		
	/*
	 * Look at the transmit status.  If there's a problem then
	 * deal with it.
	 */
	st = c509_read8(C509_W1_TX_STATUS);
	
	/*
	 * If we've had a jabber or underrun error we need a reset.
	 */
	if (st & 0x30) {
		c509_write16(C509_CMD, CMD_TX_RESET);
	}
	
	/*
	 * If we've had a jabber, underrun, maximum collisions
	 * or tx status overflow we need to re-enable.
	 */
	if (st & 0x3c) {
		c509_write16(C509_CMD, CMD_TX_ENABLE);
	}

	/*
	 * Look at the transmit free bytes.
	 */
	st = c509_read16(C509_W1_TX_FREE);
	if(st < count)
		return -1;

	/*
	 * Write out the 32 bit header at the start of the packet buffer.--
	 */
	c509_write16(C509_W1_TX_DATA, 0x0000 | count);
	c509_write16(C509_W1_TX_DATA, 0x0000);

	/*
	 * Write out the different sections of the network buffer.  Note that
	 * we make the rather sweeping assumption that only the application
	 * layer has any odd-sized segments.
	 */
	words = count >> 1;
	while (words) {
		c509_write16(C509_W1_TX_DATA, *(u16_t *)buf);
		buf += sizeof(u16_t);
		words--;
	}
	
	/* Padding to double word */
	if (count & 0x1) {
		c509_write8(C509_W1_TX_DATA, *(u8_t *)buf);
		c509_write8(C509_W1_TX_DATA, 0x00);
	}
	if (((count + 1) >> 1) & 0x1) {
		c509_write16(C509_W1_TX_DATA, 0x00);
	}

	return count;
}
