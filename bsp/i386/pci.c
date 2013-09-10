/*
 * pci.c
 * ffxz
 * 2002-7-30
 */
#include <sys/types.h>
#include <pci.h>
#include <stdio.h>
#include <memory.h>
#include <bsp.h>

#include "i386_hw.h"

#undef PCI_DEBUG

#define PCI_MAXDEVICE 32;

#define PCI_ADDR_PORT    0x0cf8
#define PCI_DATA_PORT    0x0cfc

#define PCI_ENABLE       0x80000000ul
#define PCI_ENABLE_CHK   0x80000000ul
#define PCI_ENABLE_MSK   0x7ff00000ul
#define PCI_ENABLE_CHK1  0xff000001ul
#define PCI_ENABLE_MSK1  0x80000001ul
#define PCI_ENABLE_RES1  0x80000000ul

#define	MAX_PCI_DEVICES	16

static int g_pci_devices_num = 0;
static struct pci_entry g_pci_devices[MAX_PCI_DEVICES];

#ifdef PCI_DEBUG
void pci_dump( struct pci_entry* info )
{
	kprintf( "----------------------------\n" );
	kprintf( "Vendor          = %04x\n",  info->vendor );
	kprintf( "Device          = %04x\n",  info->device_id );
	kprintf( "Revsion         = %01x\n",  info->revision_id );
	kprintf( "Base0           = %08lx\n", info->u.h0.base0 );
	kprintf( "Base1           = %08lx\n", info->u.h0.base1 );
	kprintf( "Base2           = %08lx\n", info->u.h0.base2 );
	kprintf( "Base3           = %08lx\n", info->u.h0.base3 );
	kprintf( "Base4           = %08lx\n", info->u.h0.base4 );
	kprintf( "Base5           = %08lx\n", info->u.h0.base5 );
	kprintf( "InterruptLine   = %01x\n",  info->u.h0.interrupt_line );
	kprintf( "InterruptPin    = %01x\n",  info->u.h0.interrupt_pin );
	kprintf( "\n" );
}
#endif /* PCI_DEBUG */

static int pci_inst_check( void )
{
	return 1;
}

uint32_t read_pci_config( char bus, char device, char function, char offset, char size )
{
	unsigned long result = 0xffff;
	unsigned long cfg;
	unsigned long addr;

	if ( size == 4 || size == 2 || size == 1 )
	{
		cfg = 0;
		cfg = PCI_ENABLE
			| (((unsigned long) bus   ) << 16ul)
			| (((unsigned long) device ) << 11ul)
			| (((unsigned long) function  ) <<  8ul);
		
		addr = cfg | ( (size==4? offset : offset & ~0x03) & 0xfc );
		outl (addr, PCI_ADDR_PORT);
		result = inl (PCI_DATA_PORT);
		outl (0, PCI_ADDR_PORT);
	}

	if ( size ==1 )
	{
		switch (offset & 0x3)
		{
		case 0:
			result &= 0xff;
			break;
		case 1:
			result = (result >> 8) & 0xff;
			break;
		case 2:
			result = (result >> 16) & 0xff;
			break;
		case 3:
			result = (result >> 24) & 0xff;
			break;
		}
	}
	else if ( size ==2 )
	{
		if (offset & 0x2) result = (result >> 16) & 0xffff;
		else result = result & 0xffff;
	}

	return result;
}

uint32_t write_pci_config( char bus, char device, char function, char offset, char size, unsigned int value )
{
	unsigned long result =0;
	unsigned long cfg;
	unsigned long addr;

	if ( size == 4 || size == 2 || size == 1 )
	{
		cfg = 0;
		cfg = PCI_ENABLE
			| (((unsigned long) bus   ) << 16ul)
			| (((unsigned long) device) << 11ul)
			| (((unsigned long) function  ) <<  8ul);

		addr = cfg | ( (size==4? offset : offset & ~0x03) & 0xfc );
		outl (addr, PCI_ADDR_PORT);
		outl (value, PCI_DATA_PORT);
		outl (0, PCI_ADDR_PORT);
	}

	return result;
}

static int read_pci_header( struct pci_entry* info, int bus, int dev, int fnc )
{
	info->bus		= bus;
	info->device	= dev;
	info->function	= fnc;

	info->vendor 		= read_pci_config( bus, dev, fnc, PCI_VENDOR_ID, 2 );
	info->device_id 	= read_pci_config( bus, dev, fnc, PCI_DEVICE_ID, 2 );
	info->command 		= read_pci_config( bus, dev, fnc, PCI_COMMAND, 2 );
	info->status 		= read_pci_config( bus, dev, fnc, PCI_STATUS, 2 );
	info->revision_id	= read_pci_config( bus, dev, fnc, PCI_REVISION, 1 );

	info->cache_line_size= read_pci_config( bus, dev, fnc, PCI_LINE_SIZE, 1 );
	info->latency_timer = read_pci_config( bus, dev, fnc, PCI_LATENCY, 1 );
	info->header_type 	= read_pci_config( bus, dev, fnc, PCI_HEADER_TYPE, 1 );

	info->u.h0.base0	= read_pci_config( bus, dev, fnc, PCI_BASE_REGISTERS + 0, 4 );
	info->u.h0.base1	= read_pci_config( bus, dev, fnc, PCI_BASE_REGISTERS + 4, 4 );
	info->u.h0.base2	= read_pci_config( bus, dev, fnc, PCI_BASE_REGISTERS + 8, 4 );
	info->u.h0.base3	= read_pci_config( bus, dev, fnc, PCI_BASE_REGISTERS + 12, 4 );
	info->u.h0.base4	= read_pci_config( bus, dev, fnc, PCI_BASE_REGISTERS + 16, 4 );
	info->u.h0.base5	= read_pci_config( bus, dev, fnc, PCI_BASE_REGISTERS + 20, 4 );

	info->u.h0.CIS_pointer		= read_pci_config( bus, dev, fnc, PCI_CARDBUS_CIS, 4 );
	info->u.h0.sub_sys_vendor	= read_pci_config( bus, dev, fnc, PCI_SUBSYSTEM_VENDOR_ID, 2 );
	info->u.h0.sub_sys_id		= read_pci_config( bus, dev, fnc, PCI_SUBSYSTEM_ID, 2 );
	info->u.h0.EPROM_addr		= read_pci_config( bus, dev, fnc, PCI_ROM_BASE, 4 );
	info->u.h0.capability_list	= read_pci_config( bus, dev, fnc, PCI_CAPABILITY_LIST, 1 );
	info->u.h0.interrupt_line	= read_pci_config( bus, dev, fnc, PCI_INTERRUPT_LINE, 1 );
	info->u.h0.interrupt_pin	= read_pci_config( bus, dev, fnc, PCI_INTERRUPT_PIN, 1 );
	info->u.h0.MinDMA_time		= read_pci_config( bus, dev, fnc, PCI_MIN_GRANT, 1 );
	info->u.h0.MaxDMA_latency	= read_pci_config( bus, dev, fnc, PCI_MAX_LATENCY, 1 );
	return( 0 );
}

int get_pci_info( struct pci_info* info, int index )
{
	if ( info == NULL ) return( -1 );

	if ( index >= 0 && index < g_pci_devices_num )
	{
		struct pci_entry* entry = &g_pci_devices[index];

		if ( entry == NULL ) return -1;

		info->bus			= entry->bus;
		info->device		= entry->device;
		info->function		= entry->function;

		info->vendor 		= entry->vendor;
		info->device_id 	= entry->device_id;
		info->revision_id	= entry->revision_id;

		info->cache_line_size	= entry->cache_line_size;
		info->header_type		= entry->header_type;


		info->u.h0.base0	= entry->u.h0.base0;
		info->u.h0.base1	= entry->u.h0.base1;
		info->u.h0.base2	= entry->u.h0.base2;
		info->u.h0.base3	= entry->u.h0.base3;
		info->u.h0.base4	= entry->u.h0.base4;
		info->u.h0.base5	= entry->u.h0.base5;

		info->u.h0.interrupt_line		= entry->u.h0.interrupt_line;
		info->u.h0.interrupt_pin		= entry->u.h0.interrupt_pin;
		info->u.h0.MinDMA_time			= entry->u.h0.MinDMA_time;
		info->u.h0.MaxDMA_latency		= entry->u.h0.MaxDMA_latency;

		return( 0 );
	}
	else 
		return( -1 );
}

void pci_init()
{
	int	dev;
	int	bus;
	int	header_type = 0;
	int	fnc;
	unsigned int vendor;
	struct pci_entry* info;
	int	bus_count = pci_inst_check();

	if ( bus_count != -1 )
	{
#ifdef PCI_DEBUG
		kprintf( "Vend Dev  Rev Base0    Base1    Base2    Base3    Base4    Base5  irq-l irq-p\n" );
#endif /* PCI_DEBUG */
		for ( bus = 0 ; bus < bus_count ; ++bus )
		{
			for ( dev = 0 ; dev < 32 ; ++ dev )
			{
				for ( fnc = 0 ; fnc < 8 ; ++fnc )
				{
					vendor = read_pci_config( bus, dev, fnc, PCI_VENDOR_ID, 2 );

					if ( vendor != 0xffff )
					{
						if ( fnc == 0 ) 
							header_type = read_pci_config( bus, dev, fnc, PCI_HEADER_TYPE, 1 );
						else 
							if ( (header_type & PCI_MULTIFUNCTION) == 0 ) 
								continue;

						info = &g_pci_devices[g_pci_devices_num];
						g_pci_devices_num++;

						read_pci_header( info, bus, dev, fnc );
#ifdef PCI_DEBUG
						kprintf( "%04x %04x %03x %08lx %08lx %08lx %08lx %08lx %08lx %01x    %01x\n",
							info->vendor, info->device_id, info->revision_id,
							info->u.h0.base0, info->u.h0.base1, info->u.h0.base2,
							info->u.h0.base3, info->u.h0.base4, info->u.h0.base5,
							info->u.h0.interrupt_line, info->u.h0.interrupt_pin );
#endif /* PCI_DEBUG */
						if ( g_pci_devices_num > MAX_PCI_DEVICES )
						{
							kprintf( "WARNING : To many PCI devices!\n" );
							return;
						}
					}
				}
			}
		}
#ifdef PCI_DEBUG
		kprintf( "Found %d PCI devices\n", g_pci_devices_num );
#endif /* PCI_DEBUG */
	}
#ifdef PCI_DEBUG
	else
	{
		kprintf( "Unable to read PCI information\n" );
	}
#endif /* PCI_DEBUG */
}

