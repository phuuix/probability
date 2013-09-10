/*
 * (C) Copyright 2000
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

OUTPUT_FORMAT("elf32-littlearm", "elf32-littlearm", "elf32-littlearm")
OUTPUT_ARCH(arm)
ENTRY(Reset_Handler)

/* Highest address of the user mode stack */
_estack = 0x20020000;    /* end of 128K SRAM */

SECTIONS
{
	. = 0x00000000;
	
	/* The startup code goes first into FLASH */
	.isr_vector :
	{
		. = ALIGN(4);
		KEEP(*(.isr_vector)) /* Startup code */
		. = ALIGN(4);
	}

	. = ALIGN(4);
	.text      :
	{
		startup_stm32f4xx.o	(.text*)
		*(.text)
		*(.text*)          /* .text* sections (code) */

		. = ALIGN(4);
		_etext = .;        /* define a global symbols at end of code */
	}

	. = ALIGN(4);
	.rodata : { *(.rodata) }

	/* used by the startup to initialize data */
	_sidata = .;
	. = 0x20000000;
	/* Initialized data sections goes into RAM, load LMA copy after code */
	.data : AT ( _sidata )
	{
		. = ALIGN(4);
		_sdata = .;        /* create a global symbol at data start */
		*(.data)           /* .data sections */
		*(.data*)          /* .data* sections */

		. = ALIGN(4);
		_edata = .;        /* define a global symbol at data end */
	}

	. = ALIGN(4);
	.got : { *(.got) }

	. = ALIGN(4);
	_sbss = .;         /* define a global symbol at bss start */
	__bss_start__ = _sbss;
	.bss :
	{
		/* This is used by the startup in order to initialize the .bss secion */
		
		*(.bss)
		*(.bss*)
	}
	. = ALIGN(4);
	_ebss = .;         /* define a global symbol at bss end */
	__bss_end__ = _ebss;
	
	armboot_end = .;
}
