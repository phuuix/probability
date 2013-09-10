/*
 * rtl8139.c
 * Driver for RTL8139 PCI ethernet card.
 */
#include <config.h>
#include <pci.h>
#include <bsp.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <assert.h>
#include <irq.h>
#include <isr.h>
#include <ipc.h>

#include <net/netintf.h>
#include <net/net_task.h>
#include <net/in.h>

#include "i386_hw.h"
#include "rtl8139.h"

#ifdef IP_STACK_LWIP
extern int lwip_buffer_copy_to(char *, int, void *);
extern void *lwip_buffer_alloc(int);
extern void lwip_buffer_copy_from(void *, int, void *);
extern void lwip_ethpkt_notify(void *);
#endif

#define RTL8139_DEBUG
//#define RTL8139_DEBUG_DATA

/* Size of the in-memory receive ring. */
#define RX_BUF_LEN_IDX	0		/* 0==8K, 1==16K, 2==32K, 3==64K */
#define RX_BUF_LEN (8192 << RX_BUF_LEN_IDX)
#define RX_READ_POINTER_MASK ((RX_BUF_LEN - 1) & ~3)

/* Size of the Tx bounce buffers -- must be at least (dev->mtu+14+4). */
#define TX_BUF_SIZE	1536

/* The following settings are log_2(bytes)-4:  0 == 16 bytes .. 6==1024. */
#define RX_FIFO_THRESH	4		/* Rx buffer level before first PCI xfer.  */
#define RX_DMA_BURST	4		/* Maximum PCI burst, '4' is 256 bytes */
#define TX_DMA_BURST	4		/* Calculate as 16<<val. */
#define RX_AB	0x08
#define RX_AM	0x04
#define RX_APM	0x02

#define NUM_TX_DESC		4			/* Number of Tx descriptor registers. */
#define MAX_ADDR_LEN	6			/* Largest hardware address length */

enum rtl8139_registers
{
	MAC0=0,						/* Ethernet hardware address. */
	MAR0=8,						/* Multicast filter. */
	TxStatus0=0x10,				/* Transmit status (Four 32bit registers). */
	TxAddr0=0x20,				/* Tx descriptors (also four 32bit). */
	RxBuf=0x30, RxEarlyCnt=0x34, RxEarlyStatus=0x36,
	ChipCmd=0x37, RxBufPtr=0x38, RxBufAddr=0x3A,
	IntrMask=0x3C, IntrStatus=0x3E,
	TxConfig=0x40, RxConfig=0x44,
	Timer=0x48,					/* A general-purpose counter. */
	RxMissed=0x4C,				/* 24 bits valid, write clears. */
	Cfg9346=0x50, Config0=0x51, Config1=0x52,
	FlashReg=0x54, GPPinData=0x58, GPPinDir=0x59, MII_SMI=0x5A, HltClk=0x5B,
	MultiIntr=0x5C, TxSummary=0x60,
	MII_BMCR=0x62, MII_BMSR=0x64, NWayAdvert=0x66, NWayLPAR=0x68,
	NWayExpansion=0x6A,
	/* Undocumented registers, but required for proper operation. */
	FIFOTMS=0x70,				/* FIFO Test Mode Select */
	CSCR=0x74,					/* Chip Status and Configuration Register. */
	PARA78=0x78, PARA7c=0x7c,	/* Magic transceiver parameter register. */
};

enum ChipCmdBits {
	CmdReset=0x10, CmdRxEnb=0x08, CmdTxEnb=0x04, RxBufEmpty=0x01, };

/* Interrupt register bits, using my own meaningful names. */
enum IntrStatusBits {
	PCIErr=0x8000, PCSTimeout=0x4000,
	RxFIFOOver=0x40, RxUnderrun=0x20, RxOverflow=0x10,
	TxErr=0x08, TxOK=0x04, RxErr=0x02, RxOK=0x01,
};
enum TxStatusBits {
	TxHostOwns=0x2000, TxUnderrun=0x4000, TxStatOK=0x8000,
	TxOutOfWindow=0x20000000, TxAborted=0x40000000, TxCarrierLost=0x80000000,
};
enum RxStatusBits {
	RxMulticast=0x8000, RxPhysical=0x4000, RxBroadcast=0x2000,
	RxBadSymbol=0x0020, RxRunt=0x0010, RxTooLong=0x0008, RxCRCErr=0x0004,
	RxBadAlign=0x0002, RxStatusOK=0x0001,
};

enum CSCRBits {
	CSCR_LinkOKBit=0x0400, CSCR_LinkChangeBit=0x0800,
	CSCR_LinkStatusBits=0x0f000, CSCR_LinkDownOffCmd=0x003c0,
	CSCR_LinkDownCmd=0x0f3c0,
};	

struct net_device_stats
{
	unsigned long rx_packets;	/* total packets received	*/
	unsigned long tx_packets;	/* total packets transmitted	*/
	unsigned long rx_errors;	/* receive bad packets */
	unsigned long tx_errors;	/* transmit bad packets */
};

struct rtl8139_device
{
	char devname[8];

	unsigned short flags;

	unsigned long base_addr;	/* device I/O address	*/
	unsigned int irq;			/* device IRQ number	*/

	int chip_id;
	int chip_revision;
	unsigned char pci_bus, pci_device, pci_function;
	struct net_device_stats stats;

	unsigned int cur_rx;		/* Index into the Rx buffer of next Rx pkt. */
	unsigned int cur_tx;		/* Index into the next descriptor. */
	unsigned char free_tx_desc;	/* Free Tx descriptor number */

	/* The saved address of a sent-in-place packet/buffer, for skfree(). */
	unsigned char *tx_buf[NUM_TX_DESC];	/* Tx bounce buffers */

	unsigned char *rx_ring;
	unsigned char *tx_bufs;				/* Tx bounce buffer region. */

	char phys[4];						/* MII device addresses. */
	unsigned int tx_full:1;				/* The Tx queue is full. */
	unsigned int full_duplex:1;			/* Full-duplex operation requested. */
	unsigned int duplex_lock:1;			/* Full-duplex operation requested. */
	unsigned int default_port:4;		/* Last dev->if_port value. */

	/* Interface address info. */
	unsigned char dev_addr[MAX_ADDR_LEN];	/* hw address	*/
};

struct rtl8139_device g_rtldev;

struct ifnet rtl8139_ifnet =
{
	rtl8139_init, rtl8139_cleanup, rtl8139_open, rtl8139_close,
	rtl8139_ioctl, rtl8139_recv, rtl8139_send, rtl8139_poll_recv, rtl8139_poll_send
};

/* This table drives the PCI probe routines.  It's mostly boilerplate in all
   of the drivers, and will likely be provided by some future kernel.
   Note the matching code -- the first table entry matchs all 56** cards but
   second only the 1234 card.
*/
enum pci_flags_bit
{
	PCI_USES_IO=1, PCI_USES_MEM=2, PCI_USES_MASTER=4,
	PCI_ADDR0=0x10<<0, PCI_ADDR1=0x10<<1, PCI_ADDR2=0x10<<2, PCI_ADDR3=0x10<<3,
};

struct pci_id_info
{
	const char *name;
	unsigned short vendor_id, device_id, device_id_mask, flags;
	int io_size;
};

static struct pci_id_info pci_tbl[] =
{
	{ "RealTek RTL8129 Fast Ethernet",
		0x10ec, 0x8129, 0xffff, PCI_USES_IO|PCI_USES_MASTER, 0x80},
	{ "RealTek RTL8139 Fast Ethernet",
		0x10ec, 0x8139, 0xffff, PCI_USES_IO|PCI_USES_MASTER, 0x80},
	{ "RealTek RTL8139 Fast Ethernet (mislabeled)",
		0x1113, 0x1211, 0xffff, PCI_USES_IO|PCI_USES_MASTER, 0x80},
	{0,},						/* 0 terminated list. */
};

/* The capability table matches the chip table above. */
enum {HAS_MII_XCVR=0x01, HAS_CHIP_XCVR=0x02, HAS_LNK_CHNG=0x04};
static int rtl_cap_tbl[] =
{
	HAS_MII_XCVR, HAS_CHIP_XCVR|HAS_LNK_CHNG, HAS_CHIP_XCVR|HAS_LNK_CHNG,
};

/* buffer */
/* TODO: Allocate buffer dynamic */
static unsigned char rx_buffer[RX_BUF_LEN + 16];
static unsigned char tx_buffer[TX_BUF_SIZE * NUM_TX_DESC];

static void rtl8139_isr(int n);
static void rtl8139_rx_job(void *param);
static void rtl8139_isr_job(void *param);
static void rtl8139_rxerr_job(void *param);

/* Serial EEPROM section. */

/*  EEPROM_Ctrl bits. */
#define EE_SHIFT_CLK	0x04	/* EEPROM shift clock. */
#define EE_CS			0x08	/* EEPROM chip select. */
#define EE_DATA_WRITE	0x02	/* EEPROM chip data in. */
#define EE_WRITE_0		0x00
#define EE_WRITE_1		0x02
#define EE_DATA_READ	0x01	/* EEPROM chip data out. */
#define EE_ENB			(0x80 | EE_CS)

/* Delay between EEPROM clock transitions.
 * No extra delay is needed with 33Mhz PCI, but 66Mhz may change this.
 */
#define eeprom_delay()	inl(ee_addr)

#define EE_WRITE_CMD	(5 << 6)
#define EE_READ_CMD		(6 << 6)
#define EE_ERASE_CMD	(7 << 6)

static int read_eeprom(long ioaddr, int location)
{
	int i;
	unsigned retval = 0;
	long ee_addr = ioaddr + Cfg9346;
	int read_cmd = location | EE_READ_CMD;

	outb(EE_ENB & ~EE_CS, ee_addr);
	outb(EE_ENB, ee_addr);

	/* Shift the read command bits out. */
	for (i = 10; i >= 0; i--)
	{
		int dataval = (read_cmd & (1 << i)) ? EE_DATA_WRITE : 0;
		outb(EE_ENB | dataval, ee_addr);
		eeprom_delay();
		outb(EE_ENB | dataval | EE_SHIFT_CLK, ee_addr);
		eeprom_delay();
	}
	outb(EE_ENB, ee_addr);
	eeprom_delay();

	for (i = 16; i > 0; i--)
	{
		outb(EE_ENB | EE_SHIFT_CLK, ee_addr);
		eeprom_delay();
		retval = (retval << 1) | ((inb(ee_addr) & EE_DATA_READ) ? 1 : 0);
		outb(EE_ENB, ee_addr);
		eeprom_delay();
	}

	/* Terminate the EEPROM access. */
	outb(~EE_CS, ee_addr);
	return retval;
}

/* MII serial management: mostly bogus for now. */
/* Read and write the MII management registers using software-generated
   serial MDIO protocol.
   The maximum data clock rate is 2.5 Mhz.  The minimum timing is usually
   met by back-to-back PCI I/O cycles, but we insert a delay to avoid
   "overclocking" issues. */
#define MDIO_DIR		0x80
#define MDIO_DATA_OUT	0x04
#define MDIO_DATA_IN	0x02
#define MDIO_CLK		0x01
#define MDIO_WRITE0 (MDIO_DIR)
#define MDIO_WRITE1 (MDIO_DIR | MDIO_DATA_OUT)

#define mdio_delay()	inb(mdio_addr)

static char mii_2_8139_map[8] = {MII_BMCR, MII_BMSR, 0, 0, NWayAdvert,
								 NWayLPAR, NWayExpansion, 0 };

/* Syncronize the MII management interface by shifting 32 one bits out. */
static void mdio_sync(long mdio_addr)
{
	int i;

	for (i = 32; i >= 0; i--)
	{
		outb(MDIO_WRITE1, mdio_addr);
		mdio_delay();
		outb(MDIO_WRITE1 | MDIO_CLK, mdio_addr);
		mdio_delay();
	}
	return;
}

static int mdio_read(struct rtl8139_device *dev, int phy_id, int location)
{
	long mdio_addr = dev->base_addr + MII_SMI;
	int mii_cmd = (0xf6 << 10) | (phy_id << 5) | location;
	int retval = 0;
	int i;

	if ((phy_id & 0x1f) == 0)
	{	/* Really a 8139.  Use internal registers. */
		return location < 8 && mii_2_8139_map[location] ?
			inw(dev->base_addr + mii_2_8139_map[location]) : 0;
	}
	mdio_sync(mdio_addr);
	/* Shift the read command bits out. */
	for (i = 15; i >= 0; i--)
	{
		int dataval = (mii_cmd & (1 << i)) ? MDIO_DATA_OUT : 0;

		outb(MDIO_DIR | dataval, mdio_addr);
		mdio_delay();
		outb(MDIO_DIR | dataval | MDIO_CLK, mdio_addr);
		mdio_delay();
	}

	/* Read the two transition, 16 data, and wire-idle bits. */
	for (i = 19; i > 0; i--)
	{
		outb(0, mdio_addr);
		mdio_delay();
		retval = (retval << 1) | ((inb(mdio_addr) & MDIO_DATA_IN) ? 1 : 0);
		outb(MDIO_CLK, mdio_addr);
		mdio_delay();
	}

	return (retval>>1) & 0xffff;
}

static void mdio_write(struct rtl8139_device *dev, int phy_id, int location, int value)
{
	long mdio_addr = dev->base_addr + MII_SMI;
	int mii_cmd = (0x5002 << 16) | (phy_id << 23) | (location<<18) | value;
	int i;

	if (phy_id == 32)
	{			/* Really a 8139.  Use internal registers. */
		if (location < 8  &&  mii_2_8139_map[location]) outw(value, dev->base_addr + mii_2_8139_map[location]);
		return;
	}
	mdio_sync(mdio_addr);

	/* Shift the command bits out. */
	for (i = 31; i >= 0; i--)
	{
		int dataval = (mii_cmd & (1 << i)) ? MDIO_WRITE1 : MDIO_WRITE0;
		outb(dataval, mdio_addr);
		mdio_delay();
		outb(dataval | MDIO_CLK, mdio_addr);
		mdio_delay();
	}
	
	/* Clear out extra bits. */
	for (i = 2; i > 0; i--)
	{
		outb(0, mdio_addr);
		mdio_delay();
		outb(MDIO_CLK, mdio_addr);
		mdio_delay();
	}
	return;
}

void rtl8139_init()
{
	int ioaddr;
	int irq;
	struct pci_info info;
	int cards_found = 0;
	int chip_idx =0;
	int i;
	int option = 0;
	unsigned char config1;

	g_rtldev.flags = 0;

	for ( i = 0 ; get_pci_info( &info, i ) == 0 ; ++i )
	{
		int pci_command;
		int new_command;
		for ( chip_idx = 0 ; pci_tbl[chip_idx].vendor_id ; chip_idx++ )
		{
			if ( info.vendor == pci_tbl[chip_idx].vendor_id &&
				 (info.device_id & pci_tbl[chip_idx].device_id_mask) == pci_tbl[chip_idx].device_id)
			{
				break;
			}
		}

		if (pci_tbl[chip_idx].vendor_id == 0) continue;

		g_rtldev.base_addr = info.u.h0.base0 & PCI_ADDRESS_IO_MASK;
		g_rtldev.irq = info.u.h0.interrupt_line;

		pci_command = read_pci_config( info.bus, info.device, info.function, PCI_COMMAND, 2 );
		new_command = pci_command | (pci_tbl[chip_idx].flags & 7);
		if (pci_command != new_command)
		{
			kprintf("The PCI BIOS has not enabled the"
				" device at %d/%d/%d!  Updating PCI command %4.4x->%4.4x.\n",
				info.bus, info.device, info.function, pci_command, new_command );
			write_pci_config( info.bus, info.device, info.function, PCI_COMMAND, 2, new_command);
		}

		cards_found++;
		break;
	}

	if ( !cards_found )
	{
		#ifdef RTL8139_DEBUG
		kprintf("Can't find a rtl8139 network cards.\n");
		#endif /* RTL8139_DEBUG */
		return;
	}

	ioaddr = g_rtldev.base_addr;
	irq = g_rtldev.irq;
	
#ifdef RTL8139_DEBUG
	kprintf("RTL8139 ioaddr=%x, irq=%d\n", ioaddr, irq);
#endif
	
	/* Bring the chip out of low-power mode. */
	config1 = inb(ioaddr + Config1);
	if (rtl_cap_tbl[chip_idx] & HAS_MII_XCVR)			/* rtl8129 chip */
		outb(config1 & ~0x03, ioaddr + Config1);
	
	if (read_eeprom(ioaddr, 0) != 0xffff)
	{
		for (i = 0; i < 3; i++)
			((unsigned short *)&(g_rtldev.dev_addr))[i] = read_eeprom(ioaddr, i + 7);
	}
	else
	{
		for (i = 0; i < 6; i++)
			g_rtldev.dev_addr[i] = inb(ioaddr + MAC0 + i);
	}

#ifdef RTL8139_DEBUG	
	kprintf("RTL8139 Mac address is");
	for( i=0; i<6; i++)
	    kprintf(":%02x", g_rtldev.dev_addr[i]);
	kprintf("\n");
#endif

	g_rtldev.chip_id		= chip_idx;
	g_rtldev.pci_bus		= info.bus;
	g_rtldev.pci_device		= info.device;
	g_rtldev.pci_function	= info.function;

	if (rtl_cap_tbl[chip_idx] & HAS_MII_XCVR)
	{
		int phy, phy_idx;

		#ifdef RTL8139_DEBUG
		kprintf("rtl8139 HAS_MII_XCVR\n");
		#endif
		
		for (phy = 0, phy_idx = 0; phy < 32 && phy_idx < sizeof(g_rtldev.phys); phy++)
		{
			int mii_status = mdio_read(&g_rtldev, phy, 1);
			if (mii_status != 0xffff  && mii_status != 0x0000)
			{
				g_rtldev.phys[phy_idx++] = phy;
				#ifdef RTL8139_DEBUG
				kprintf("%s: MII transceiver found at address %d.\n", g_rtldev.devname, phy);
				#endif
			}
		}
		if (phy_idx == 0)
		{
			#ifdef RTL8139_DEBUG
			kprintf("%s: No MII transceivers found!  Assuming SYM transceiver.\n", g_rtldev.devname);
			#endif
			g_rtldev.phys[0] = -1;
		}
	}
	else
	{
		g_rtldev.phys[0] = 32;
	}

	/* Put the chip into low-power mode. */
	outb(0xC0, ioaddr + Cfg9346);
	if (rtl_cap_tbl[chip_idx] & HAS_MII_XCVR)
		outb(0x03, ioaddr + Config1);
	outb('H', ioaddr + HltClk);		/* 'R' would leave the clock running. */

	/* The lower four bits are the media type. */
	if (option > 0)
	{
		g_rtldev.full_duplex = (option & 0x200) ? 1 : 0;
		g_rtldev.default_port = option & 15;
	}

	if (g_rtldev.full_duplex)
	{
		#ifdef RTL8139_DEBUG
		kprintf("RTL8139 Media type forced to Full Duplex\n", g_rtldev.devname);
		#endif
		mdio_write(&g_rtldev, g_rtldev.phys[0], 4, 0x141);
		g_rtldev.duplex_lock = 1;
	}

	/* reset rtl8139 */
	outb(CmdReset, ioaddr + ChipCmd);

	g_rtldev.tx_bufs = tx_buffer;
	g_rtldev.rx_ring = rx_buffer;

	assert(g_rtldev.tx_bufs);
	assert(g_rtldev.rx_ring);
	assert(((int)tx_buffer & 3) == 0);
	assert(((int)rx_buffer & 3) == 0);
	
	g_rtldev.cur_rx = 0;
	g_rtldev.cur_tx = 0;
	g_rtldev.free_tx_desc = NUM_TX_DESC;

	for (i = 0; i < NUM_TX_DESC; i++)
	{
		g_rtldev.tx_buf[i] = &g_rtldev.tx_bufs[i*TX_BUF_SIZE];
	}

	/* Check that the chip has finished the reset. */
	for (i = 1000; i > 0; i--)
	{
		if ((inb(ioaddr + ChipCmd) & CmdReset) == 0) break;
	}
	
	/* setup the MAC address */
	for (i = 0; i < 6; i++) outb(g_rtldev.dev_addr[i], ioaddr + MAC0 + i);

	/* Must enable Tx/Rx before setting transfer thresholds! */
	outb(CmdRxEnb | CmdTxEnb, ioaddr + ChipCmd);
	outl(RX_AB | RX_AM | RX_APM | 
		(RX_FIFO_THRESH << 13) | 
		(RX_BUF_LEN_IDX << 11) | 
		(RX_DMA_BURST<<8), ioaddr + RxConfig);
	outl((TX_DMA_BURST<<8)|0x03000000, ioaddr + TxConfig);

	g_rtldev.full_duplex = g_rtldev.duplex_lock;
	if(g_rtldev.phys[0] >= 0 || (rtl_cap_tbl[g_rtldev.chip_id] & HAS_MII_XCVR))
	{
		unsigned short mii_reg5 = mdio_read(&g_rtldev, g_rtldev.phys[0], 5);
		if (mii_reg5 == 0xffff)
			;
		else 
		if ((mii_reg5 & 0x0100) == 0x0100 || (mii_reg5 & 0x00C0) == 0x0040)
		{
			g_rtldev.full_duplex = 1;
		}
	}

//	outb(0xC0, ioaddr + Cfg9346);
	if (rtl_cap_tbl[chip_idx] & HAS_MII_XCVR)
		outb(g_rtldev.full_duplex ? 0x60 : 0x20, ioaddr + Config1);
	outb(0x00, ioaddr + Cfg9346);

	outl((unsigned int)(g_rtldev.rx_ring), ioaddr + RxBuf);

	/* Start the chip's Tx and Rx process. */
	/* reset the Rx Missed Counter. */
	outl(0, ioaddr + RxMissed);

	outb(CmdRxEnb | CmdTxEnb, ioaddr + ChipCmd);
	
	/* Enable all known interrupts by setting the interrupt mask. */
	outw(PCIErr | PCSTimeout | RxUnderrun | RxOverflow | RxFIFOOver
		 | TxErr | TxOK | RxErr | RxOK, ioaddr + IntrMask);

	/* setup isr handle */
	bsp_isr_attach(irq, rtl8139_isr);
	bsp_irq_unmask(irq);

}


/* standard driver function: ioctl() */
int rtl8139_ioctl(int d, int request, void* p)
{
	switch(request)
	{
		case NIOCTL_POLLSTART:
			rtl8139_poll_start();
			break;

		case NIOCTL_POLLSTOP:
			rtl8139_poll_stop();
			break;

		case NIOCTL_GADDR:
    		/* get mac address */
    		if(p)
				memcpy(p, g_rtldev.dev_addr, 6);
			else 
				return -1;
			break;

		default :
			break;
	}

	return 0;
}

int rtl8139_poll_start()
{
	/* Disable interrupts by clearing the interrupt mask. */
	outw( 0 , g_rtldev.base_addr + IntrMask );
	g_rtldev.flags |= FLG_POLL;

	return 0;
}

int rtl8139_poll_stop()
{
	long ioaddr;

	ioaddr = g_rtldev.base_addr;

	outw(PCIErr | PCSTimeout | RxUnderrun | RxOverflow | RxFIFOOver
		 | TxErr | TxOK | RxErr | RxOK, ioaddr + IntrMask);
	g_rtldev.flags &= ~ FLG_POLL;

#if 0
	/* stop Tx and Rx DMA processes */
	outb(0x00, ioaddr + ChipCmd);

	/* Green! Put the chip in low-power mode. */
	outb(0xC0, ioaddr + Cfg9346);
	outb(0x03, ioaddr + Config1);
	outb('H', ioaddr + HltClk);		/* 'R' would leave the clock running. */
#endif /* 0 */
	return 0;
}


/* standard driver function: poll_recv() */
int rtl8139_poll_recv(int d, char *buf, size_t count)
{
	long ioaddr;
	unsigned char *rx_ring;
	unsigned short cur_rx;
	unsigned int rx_status;
	int rx_size;
	int ring_offset;

	ioaddr = g_rtldev.base_addr;
	rx_ring= g_rtldev.rx_ring;
	cur_rx = g_rtldev.cur_rx;
	ring_offset = cur_rx % RX_BUF_LEN;

	if ( !g_rtldev.flags & FLG_POLL ) return -1;

	if(inb(ioaddr + ChipCmd) & RxBufEmpty)
	{
		/* buffer empty */
		return -1;
	}
	else
	{
		rx_status = *(unsigned int*)(rx_ring + ring_offset);
		rx_size = rx_status >> 16;

		/* Rx unfinished */
		if (rx_size == 0xfff0) 
			return -1;

		if (rx_status & (RxBadSymbol|RxRunt|RxTooLong|RxCRCErr|RxBadAlign))
		{
			kprintf("RTL8139 rx status error.\n");
			g_rtldev.stats.rx_errors++;

			/* Reset the receiver */
			g_rtldev.cur_rx = 0;
			outb(CmdRxEnb | CmdTxEnb, ioaddr + ChipCmd);
			outl(RX_AB | RX_AM | RX_APM | 
				(RX_FIFO_THRESH << 13) | 
				(RX_BUF_LEN_IDX << 11) | 
				(RX_DMA_BURST<<8), ioaddr + RxConfig);
			return -1;
		}
		else if(rx_size > count)
		{
			#ifdef RTL8139_DEBUG
			kprintf("RTL8139 rx_size(%d) lager than buffer size\n", rx_size);
			#endif
			cur_rx = (cur_rx + rx_size + 4 + 3) & ~3;
			outw(cur_rx - 16, ioaddr + RxBufPtr);
			g_rtldev.cur_rx = cur_rx;
			return -1;
		}
		else
		{
			int pkt_size = rx_size - 4;
			if (ring_offset + 4 + pkt_size > RX_BUF_LEN)
//			if (ring_offset + 4 + rx_size > RX_BUF_LEN)
			{
				int semi_count = RX_BUF_LEN - ring_offset - 4;
				memcpy(buf, &rx_ring[ring_offset + 4], semi_count);
				memcpy(buf + semi_count , rx_ring, pkt_size-semi_count);
			}
			else
			{
				memcpy(buf , &rx_ring[ring_offset + 4], pkt_size);
			}

			g_rtldev.stats.rx_packets++;

			#ifdef RTL8139_DEBUG_DATA
			kprintf("RTL8139 poll received %d bytes: \n", pkt_size);
			for(ring_offset=0; ring_offset<pkt_size; ring_offset++)
			{
				kprintf("%2.2x ", buf[ring_offset]);
			}
			kprintf("\n");
			#endif /* RTL8139_DEBUG_DATA */

			cur_rx = (cur_rx + rx_size + 4 + 3) & RX_READ_POINTER_MASK;
			outw(cur_rx - 16, ioaddr + RxBufPtr);
			g_rtldev.cur_rx = cur_rx;

			return pkt_size;
		}
	}

	return -1;
}


/* standard driver function: poll_send() */
int rtl8139_poll_send(int d, const char *buf, size_t count)
{
	long ioaddr;
	int entry;
#ifdef RTL8139_DEBUG_DATA
	int i;
#endif /* RTL8139_DEBUG_DATA */
	unsigned short stat;

	ioaddr = g_rtldev.base_addr;
	if ( !g_rtldev.flags & FLG_POLL ) return -1;
	if(count > 1514) return -1;

	outw(ioaddr + IntrStatus, 0xffff);

	/* Calculate the next Tx descriptor entry. */
	entry = g_rtldev.cur_tx % NUM_TX_DESC;

#ifdef RTL8139_DEBUG_DATA
	kprintf("rtl8139 send a frame addr(%x) size(%d) txbd(%d):\n", g_rtldev.tx_buf[entry], count, entry);
	for(i=0; i<count; i++)
	{
		kprintf("%02x ", buf[i]);
	}
	kprintf("\n");
#endif /* RTL8139_DEBUG_DATA */
	
	memcpy(g_rtldev.tx_buf[entry], buf, count);
	if(count < 60)
		count = 60;		//padding to size 60
	outl((unsigned int)g_rtldev.tx_buf[entry], ioaddr + TxAddr0 + entry*4);
	outl(count, ioaddr + TxStatus0 + entry*4);
//	outl( count + (3 << 16) , ioaddr + TxStatus0 + entry*4);
	
	while(!((stat = inw( ioaddr + IntrStatus)) & 0x04));
	
#ifdef RTL8139_DEBUG_DATA
	{
		uint32_t ret=0;
		ret = inl(ioaddr + TxStatus0 + entry*4);
		kprintf("rtl8139 tx status 0x%x intr status 0x%x\n", ret, stat);
	}
#endif /* RTL8139_DEBUG_DATA */
	
	++g_rtldev.cur_tx;
	
	return count;
}


/* standard driver function: cleanup() */
void rtl8139_cleanup()
{
}


/* standard driver function: open()*/
int rtl8139_open(const char *pathname, int flags)
{
	return 1;
}


/* standard driver function: close() */
int rtl8139_close(int d)
{
	return 0;
}


/* standard driver function: recv() */
int rtl8139_recv(int d, void *buf, size_t count)
{
	return -1;
}


/* standard driver function: send() */
int rtl8139_send(int d, const void *buf, size_t count)
{
	long ioaddr;
	int entry;
	uint32_t f;

	/* FIXME: use semaphore */
	f = bsp_fsave();

	if(g_rtldev.free_tx_desc)
	{
		#ifdef RTL8139_DEBUG_DATA
		kprintf("RTL8139 send a packet(%d).\n", count);
		#endif /* RTL8139_DEBUG */

		ioaddr = g_rtldev.base_addr;

		/* Calculate the next Tx descriptor entry. */
		entry = g_rtldev.cur_tx % NUM_TX_DESC;

		#ifdef IP_STACK_LWIP
		{
			count = 
				lwip_buffer_copy_to(g_rtldev.tx_buf[entry], TX_BUF_SIZE, (void *)buf);
//			lwip_buffer_free(buf);
		}
		#endif /* IP_STACK_LWIP */

		if(count < 60)
			count = 60;		//padding to size 60
		outl((unsigned int)g_rtldev.tx_buf[entry], ioaddr + TxAddr0 + entry*4);
		outl( count + (3 << 16) , ioaddr + TxStatus0 + entry*4);
		
		++g_rtldev.cur_tx;
		--g_rtldev.free_tx_desc;
	}
	else
		count = 0;

	bsp_frestore(f);

	return count;
}


/*
 * Interrupt service routine for RTL8139
 *   n: no use
 */
static void rtl8139_isr(int n)
{
	rtl8139_isr_job(NULL);
}

static void rtl8139_isr_job(void *param)
{
	static unsigned int jobs = 0;
	unsigned short stat;
	int ioaddr;

	ioaddr = g_rtldev.base_addr;
	
	do{
		stat = inw(ioaddr + IntrStatus);

		/* Acknowledge all of the current interrupt sources ASAP. */
		outw( (stat & RxFIFOOver) ? (stat | RxOverflow) : stat, ioaddr + IntrStatus);

		if ((stat & (PCIErr|PCSTimeout|RxUnderrun|RxOverflow|RxFIFOOver|TxErr|TxOK|RxErr|RxOK)) == 0)
			break;

		/* Clear all interrupt sources. */
		outw(stat, ioaddr + IntrStatus);
		
		if(stat & (RxOK|RxUnderrun|RxOverflow|RxFIFOOver))
		{
			jobs++;
			if(net_task_job_add(rtl8139_rx_job, NULL) != ROK)
			{
				extern mbox_t net_mbox;
				kprintf("RTL8139 net_task_job_add failed for %d.\n", jobs);
				kprintf("head=%d tail=%d unit_len=%d len=%d vlen=%d\n", net_mbox.buf.head, net_mbox.buf.tail, 
					net_mbox.buf.unit_len, net_mbox.buf.len, net_mbox.buf.vlen);
				assert(0);
			}
		}
		if(stat & (TxOK|TxErr))
		{
			int entry;
			unsigned short txstat;

			do{
				entry = (g_rtldev.cur_tx + g_rtldev.free_tx_desc - NUM_TX_DESC) & 0x3;
				txstat = inw(ioaddr + TxStatus0 + (entry<<2));

				if(txstat & (TxStatOK|TxHostOwns))
				{
					g_rtldev.stats.tx_packets++;
					g_rtldev.free_tx_desc++;
				}
				else if(txstat & (TxOutOfWindow|TxAborted))
				{
					#ifdef RTL8139_DEBUG
					kprintf("RTL8139 tx error TxOutOfWindow|TxAborted.\n");
					#endif /* RTL8139_DEBUG */

					g_rtldev.stats.tx_errors++;
					if(txstat & TxAborted){
						/* clear abort */
						outl(0x01, ioaddr + TxConfig);
					}
				}
			}
			while((txstat & (TxStatOK|TxHostOwns)) && g_rtldev.free_tx_desc<=NUM_TX_DESC);
		}
		if(stat & (PCIErr|PCSTimeout|RxUnderrun|RxOverflow|RxErr|RxFIFOOver|TxErr|RxErr))
		{
			#ifdef RTL8139_DEBUG
			static int rx_stat_err;
			kprintf("RTL8139 rx_stat(0x%x) error PCIErr|PCSTimeout|RxUnderrun|RxOverflow|RxErr|RxFIFOOver|TxErr|RxErr for %d times.\n", 
				stat, rx_stat_err++);
			#endif

			if (stat == 0xffffffff)
				break;

			/* Update the error count. */
			outl(0, ioaddr + RxMissed);

			if (stat & (RxUnderrun | RxOverflow | RxErr | RxFIFOOver))
				g_rtldev.stats.rx_errors++;

			if ((stat & RxUnderrun)  &&
				(rtl_cap_tbl[g_rtldev.chip_id] & HAS_LNK_CHNG)) {
				/* Really link-change on new chips. */
				int lpar = inw(ioaddr + NWayLPAR);
				int duplex = (lpar&0x0100)||(lpar & 0x01C0) == 0x0040; 
				if (g_rtldev.full_duplex != duplex) {
					g_rtldev.full_duplex = duplex;
					outb(0xC0, ioaddr + Cfg9346);
					outb(g_rtldev.full_duplex ? 0x60 : 0x20, ioaddr + Config1);
					outb(0x00, ioaddr + Cfg9346);
				}
			}
			if (stat & RxOverflow) {
				jobs++;
				if(net_task_job_add(rtl8139_rxerr_job, NULL) != ROK)
				{
					extern mbox_t net_mbox;
					kprintf("RTL8139 net_task_job_add failed for %d.\n", jobs);
					kprintf("head=%d tail=%d unit_len=%d len=%d vlen=%d\n", net_mbox.buf.head, net_mbox.buf.tail, 
						net_mbox.buf.unit_len, net_mbox.buf.len, net_mbox.buf.vlen);
					assert(0);
				}
			}
			if (stat & PCIErr) {
				unsigned long pci_cmd_status;
				pci_cmd_status = read_pci_config( g_rtldev.pci_bus, g_rtldev.pci_device, g_rtldev.pci_function, PCI_COMMAND, 4 );

				kprintf("PCI Bus error %4.4x.\n", pci_cmd_status);
			}
		}
	} while(1);
}


static char temp_rx_buf[1600];
static void rtl8139_rx_job(void *param)
{
	long ioaddr;
	unsigned char *rx_ring;
	unsigned short cur_rx;
	unsigned int rx_status;
	int rx_size;
	int ring_offset;

	ioaddr = g_rtldev.base_addr;
	rx_ring= g_rtldev.rx_ring;
	cur_rx = g_rtldev.cur_rx;

	while ((inb(ioaddr + ChipCmd) & 1) == 0) {
		ring_offset = cur_rx % RX_BUF_LEN;

		rx_status = *(unsigned int*)(rx_ring + ring_offset);
		rx_size = rx_status >> 16;

		if (rx_size == 0xfff0)
			break;

		if (rx_status & (RxBadSymbol|RxRunt|RxTooLong|RxCRCErr|RxBadAlign))
		{
			#ifdef RTL8139_DEBUG
			static unsigned int rx_status_err = 0;
			kprintf("rtl8139_rx_job(): rx status(0x%x) error for %d times.\n", rx_status, rx_status_err++);
			#endif
			
			g_rtldev.stats.rx_errors++;

			/* Reset the receiver */
			g_rtldev.cur_rx = 0;
			outb(CmdRxEnb | CmdTxEnb, ioaddr + ChipCmd);
			outl(RX_AB | RX_AM | RX_APM | 
				(RX_FIFO_THRESH << 13) | 
				(RX_BUF_LEN_IDX << 11) | 
				(RX_DMA_BURST<<8), ioaddr + RxConfig);
			return;
		}
		else if(rx_size > 1600)
		{
			#ifdef RTL8139_DEBUG
			static unsigned int rx_large_err = 0;
			kprintf("rtl8139_rx_job(): rx_size(%d) too large for %d times.\n", rx_size, rx_large_err++);
			#endif
			
			g_rtldev.stats.rx_errors++;

			/* Reset the receiver */
			g_rtldev.cur_rx = 0;
			outb(CmdRxEnb | CmdTxEnb, ioaddr + ChipCmd);
			outl(RX_AB | RX_AM | RX_APM | 
				(RX_FIFO_THRESH << 13) | 
				(RX_BUF_LEN_IDX << 11) | 
				(RX_DMA_BURST<<8), ioaddr + RxConfig);
			return;
		}
		else
		{
			if (ring_offset+rx_size+4 > RX_BUF_LEN)
			{
				int semi_count = RX_BUF_LEN - ring_offset - 4;
				memcpy(temp_rx_buf, &rx_ring[ring_offset + 4], semi_count);
				memcpy(temp_rx_buf + semi_count, rx_ring, rx_size-semi_count);
			}
			else
			{
				memcpy(temp_rx_buf , &rx_ring[ring_offset + 4], rx_size);
			}

			#ifdef IP_STACK_LWIP
			{
				void *pbuf;
				pbuf = (void *)lwip_buffer_alloc(rx_size);
				
				if(pbuf)
				{
					lwip_buffer_copy_from(temp_rx_buf, rx_size, pbuf);
					lwip_ethpkt_notify(pbuf);
				}
			}
			#endif /* IP_STACK_LWIP */
			g_rtldev.stats.rx_packets++;

			#ifdef RTL8139_DEBUG_DATA
			kprintf("rtl8139_rx_job(): rx_packet %d in %d bytes: \n", g_rtldev.stats.rx_packets, rx_size);
			#if 0
			for(ring_offset=0; ring_offset<rx_size; ring_offset++)
			{
				kprintf("%02x ", temp_rx_buf[ring_offset]);
			}
			kprintf("\n");
			#endif
			#endif /* RTL8139_DEBUG_DATA */

			cur_rx = (cur_rx + rx_size + 4 + 3) & ~3;
			outw(cur_rx - 16, ioaddr + RxBufPtr);

			g_rtldev.cur_rx = cur_rx;
		}
	};
}

static void rtl8139_rxerr_job(void *param)
{
	long ioaddr;
	ioaddr = g_rtldev.base_addr;

	kprintf("rtl8139_rxerr_job(): RxOverflow\n");
	g_rtldev.cur_rx = inw(ioaddr + RxBufAddr) % RX_BUF_LEN;
	outw(g_rtldev.cur_rx - 16, ioaddr + RxBufPtr);
}
