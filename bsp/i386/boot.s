/* boot.S - bootstrap the kernel */

#define ASM	1
#include <multiboot.h>
#include <linkage.h>
#include <i386_sel.h>

.data


.globl SYMBOL_NAME(IDT)
.globl SYMBOL_NAME(GDT)


/* GDT Definition */
SYMBOL_NAME_LABEL(GDT)
	.word	0,0,0,0		/* X_NULL_SEL */

	.word	0xFFFF		/* X_FLATDATA_SEL */ 
	.word	0
	.word	0x9200
	.word	0x00CF
	.word	0xFFFF		/* X_FLATCODE_SEL */ 
	.word	0
	.word	0x9A00
	.word	0x00CF
	.word	0,0,0,0		/* X_MAIN_TSS */
	.fill	256 - 4,8,0


/* IDT definition */
SYMBOL_NAME_LABEL(IDT)
	.fill	256,8,0		# idt is uninitialized

GDT_descr:
	.word 256*8-1 			# gdt contains 256 entries
	.long SYMBOL_NAME(GDT)

IDT_descr:
	.word	256*8-1		# idt contains 256 entries
	.long	SYMBOL_NAME(IDT)

	
	
	.text

	.globl	start, _start
start:
_start:
	jmp	multiboot_entry

	/* Align 32 bits boundary.  */
	.align	4
	
	/* Multiboot header.  */
multiboot_header:
	/* magic */
	.long	MULTIBOOT_HEADER_MAGIC
	/* flags */
	.long	MULTIBOOT_HEADER_FLAGS
	/* checksum */
	.long	-(MULTIBOOT_HEADER_MAGIC + MULTIBOOT_HEADER_FLAGS)
#ifndef __ELF__
	/* header_addr */
	.long	multiboot_header
	/* load_addr */
	.long	_start
	/* load_end_addr */
	.long	_edata
	/* bss_end_addr */
	.long	_end
	/* entry_addr */
	.long	multiboot_entry
#endif /* ! __ELF__ */

multiboot_entry:
	/* Initialize the stack pointer.  */
/*	movl	$(stack + 1000), %esp */
	movl	$(SYMBOL_NAME(_end) + 0x1000), %esp

	/* Reset EFLAGS.  */
	pushl	$0
	popf


	lgdt	GDT_descr

	movw	$(X_FLATDATA_SEL),%ax
	movw	%ax,%ds
	movw	%ax,%es
	movw	%ax,%ss
	movw	%ax,%fs
	movw	%ax,%gs

	lidt	IDT_descr
#if 1
	/* set cr0 to: NE, ET, MP, PE */
	movl	%cr0,%eax
	orl		$33,%eax
	movl	%eax,%cr0

	/* init fpu */
	fninit
#endif
	ljmp $X_FLATCODE_SEL, $os_begin

os_begin:

	/* Push the pointer to the Multiboot information structure.  */
	pushl	%ebx
	/* Push the magic value.  */
	pushl	%eax
	
	/* Now enter the C main function...  */
	call	SYMBOL_NAME(startup)

	/* No reached */
loop:	hlt
	jmp	loop

