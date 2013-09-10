/* linker script for Flick */

OUTPUT_FORMAT("elf32-i386")	/* we use ELF */
ENTRY(start)

phys = 0x100000;	/* GRUB can't load below 1MB */
SECTIONS
{
    . = phys;     
    .text :
    {
        *(.text)
    }     
    .rodata :
    {
	*(.rodata)	
    }
    .data :
    {
        *(.data)
    }
	__bss_start = .;	
    .bss :
    {
        bss = .;
        *(COMMON)
        *(.bss)
    }
    _end = .;
}
		
