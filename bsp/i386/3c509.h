/*
 * 3c509.h
 *	3Com 3C509 Ethernet controller definitions.
 */
#ifndef __D_3C509_H__
#define __D_3C509_H__

#include <sys/types.h>

#define C509_ID_PORT		0x110
#define C509_IO_PORT_BASE	0x210
#define C509_IRQ			10

#define C509_FLAG_POLL		0x01


/*
 * Command/status register is available on all pages.
 */
#define C509_CMD 0x0e
#define C509_STATUS 0x0e

/*
 * Window 0 registers.
 */
#define C509_W0_EEPROM_DATA 0x0c
#define C509_W0_EEPROM_CMD 0x0a
#define C509_W0_RESOURCE_CFG 0x08
#define C509_W0_CONFIG_CTRL 0x04

/*
 * Window 1 registers.
 */
#define C509_W1_TX_STATUS 0x0b
#define C509_W1_RX_STATUS 0x08
#define C509_W1_TX_DATA 0x00
#define C509_W1_RX_DATA 0x00
#define C509_W1_TX_FREE	0x0c


/*
 * Window 2 registers.
 */
#define C509_W2_ADDR5 0x05
#define C509_W2_ADDR4 0x04
#define C509_W2_ADDR3 0x03
#define C509_W2_ADDR2 0x02
#define C509_W2_ADDR1 0x01
#define C509_W2_ADDR0 0x00

/*
 * Window 3 registers.
 */
#define C509_W3_TX_FREE	0x0c
#define C509_W3_RX_FREE	0x0a


/*
 * Window 4 registers.
 */
#define C509_W4_MEDIA_TYPE 0x0a

/*
 * Commands.
 */
#define CMD_GLOBAL_RESET (0 << 11)
#define CMD_SELECT_WINDOW (1 << 11)
#define CMD_RX_ENABLE (4 << 11)
#define CMD_RX_RESET (5 << 11)
#define CMD_RX_DISCARD (8 << 11)
#define CMD_TX_ENABLE (9 << 11)
#define CMD_TX_RESET (11 << 11)
#define CMD_ACK_INT (13 << 11)
#define CMD_SET_INT_MASK (14 << 11)
#define CMD_SET_READ_ZERO_MASK (15 << 11)
#define CMD_SET_RX_FILTER (16 << 11)
#define CMD_STATS_ENABLE (21 << 11)
#define CMD_STATS_DISABLE (22 << 11)

/*
 * Status.
 */
#define STAT_INT_LATCH 0x0001
#define STAT_ADAPTER_FAILURE 0x0002
#define STAT_TX_COMPLETE 0x0004
#define STAT_TX_AVAILABLE 0x0008
#define STAT_RX_COMPLETE 0x0010
#define STAT_RX_EARLY 0x0020
#define STAT_INT_REQUESTED 0x0040
#define STAT_UPDATE_STATS 0x0080
#define STAT_CMD_IN_PROGRESS 0x1000

/*
 * c509_read8()
 *	Perform an I/O read on the Ethernet chip.
 */
#define c509_read8(reg) inp(C509_IO_PORT_BASE + reg)

/*
 * c509_read16()
 *	Perform an I/O read on the Ethernet chip.
 */
#define c509_read16(reg) inpw(C509_IO_PORT_BASE + reg)

/*
 * c509_read8id()
 *	Perform an I/O read on the ID port of the Ethernet chip.
 */
#define c509_read8id() inp(C509_ID_PORT)

/*
 * c509_write8()
 *	Perform and I/O write on the Ethernet chip.
 */
#define c509_write8(reg, data) outp(C509_IO_PORT_BASE + reg, data)

/*
 * c509_write16()
 *	Perform and I/O write on the Ethernet chip.
 */
#define c509_write16(reg, data) outpw(C509_IO_PORT_BASE + reg, data)

/*
 * c509_write8id()
 *	Perform and I/O write on the ID port of the Ethernet chip.
 */
#define c509_write8id(data) outp(C509_ID_PORT, data)

struct c509_control
{
	unsigned long flags;
	unsigned char mac_addr[6];
};

void c509_init();
void c509_cleanup();
int c509_open(const char *pathname, int flags);
int c509_close(int d);
int c509_ioctl(int d, int request, void* p);
int c509_recv(int d, void *buf, size_t count);
int c509_send(int d, const void *buf, size_t count);
int c509_poll_recv(int d, char *buf, size_t count);
int c509_poll_send(int d, const char *buf, size_t count);

#endif /* __D_3C509_H__ */
