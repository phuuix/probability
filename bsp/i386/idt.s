/*
 * IDT.S
 */

#include <linkage.h>

#define KERNEL_DEBUG

.extern SYMBOL_NAME(esr_table)
.extern SYMBOL_NAME(isr_table)

.extern SYMBOL_NAME(gdb_registers)


#ifdef KERNEL_DEBUG
#define EXC_NORMAL(n) \
.globl SYMBOL_NAME(exc##n)						; \
SYMBOL_NAME_LABEL(exc##n)						; \
	/* save registers 1 */						; \
	movl %eax, SYMBOL_NAME(gdb_registers)		; \
	movl %ecx, SYMBOL_NAME(gdb_registers)+4		; \
	movl %edx, SYMBOL_NAME(gdb_registers)+8		; \
	movl %ebx, SYMBOL_NAME(gdb_registers)+12	; \
	movl %esp, SYMBOL_NAME(gdb_registers)+16	; \
	movl %ebp, SYMBOL_NAME(gdb_registers)+20	; \
	movl %esi, SYMBOL_NAME(gdb_registers)+24	; \
	movl %edi, SYMBOL_NAME(gdb_registers)+28	; \
	/* save registers 2 */						; \
	popl %ebx	/* old eip */					; \
	movl %ebx, SYMBOL_NAME(gdb_registers)+32	; \
	popl %ebx	/* old cs */					; \
	movl %ebx, SYMBOL_NAME(gdb_registers)+40	; \
	popl %ebx	/* old eflags */				; \
	movl %ebx, SYMBOL_NAME(gdb_registers)+36	; \
	movl SYMBOL_NAME(gdb_registers)+16, %ebx	; \
	movl %esp, SYMBOL_NAME(gdb_registers)+16	; \
	movl %ebx, %esp								; \
	/* call exception handler */				; \
	pushal										; \
	movl	$##n, %eax							; \
	pushl	%eax								; \
	call	*SYMBOL_NAME(esr_table)(, %eax, 4)	; \
	popl	%eax								; \
	popal										; \
	/* restore registers 1 */					; \
	movl SYMBOL_NAME(gdb_registers)+4, %ecx		; \
	movl SYMBOL_NAME(gdb_registers)+8, %edx		; \
	movl SYMBOL_NAME(gdb_registers)+12, %ebx	; \
	movl SYMBOL_NAME(gdb_registers)+16, %esp	; \
	movl SYMBOL_NAME(gdb_registers)+20, %ebp	; \
	movl SYMBOL_NAME(gdb_registers)+24, %esi	; \
	movl SYMBOL_NAME(gdb_registers)+28, %edi	; \
	/* restore registers 2 */					; \
	sub $0xc, %esp								; \
	movl SYMBOL_NAME(gdb_registers)+32, %eax	; \
	movl %eax, 0(%esp)		/* old eip */		; \
	movl SYMBOL_NAME(gdb_registers)+40, %eax	; \
	movl %eax, 4(%esp)		/* old cs */		; \
	movl SYMBOL_NAME(gdb_registers)+36, %eax	; \
	movl %eax, 8(%esp)		/* old eflags */	; \
	movl SYMBOL_NAME(gdb_registers), %eax		; \
	iret

#else /* KERNEL_DEBUG */
#define EXC_NORMAL(n) \
.globl SYMBOL_NAME(exc##n)						; \
SYMBOL_NAME_LABEL(exc##n)						; \
	pushal										; \
	movl	$##n, %eax							; \
	jmp	exc_normal_handler
#endif /* KERNEL_DEBUG */

#ifdef KERNEL_DEBUG
#define EXC_ECODE(n) \
.globl SYMBOL_NAME(exc##n)						; \
SYMBOL_NAME_LABEL(exc##n)						; \
	/* save registers 1 */						; \
	movl %eax, SYMBOL_NAME(gdb_registers)		; \
	movl %ecx, SYMBOL_NAME(gdb_registers)+4		; \
	movl %edx, SYMBOL_NAME(gdb_registers)+8		; \
	movl %ebx, SYMBOL_NAME(gdb_registers)+12	; \
	movl %esp, SYMBOL_NAME(gdb_registers)+16	; \
	movl %ebp, SYMBOL_NAME(gdb_registers)+20	; \
	movl %esi, SYMBOL_NAME(gdb_registers)+24	; \
	movl %edi, SYMBOL_NAME(gdb_registers)+28	; \
	/* save error code */						; \
	popl %ebx									; \
	movl %ebx, SYMBOL_NAME(gdb_registers)+60	; \
	/* save registers 2 */						; \
	popl %ebx	/* old eip */					; \
	movl %ebx, SYMBOL_NAME(gdb_registers)+32	; \
	popl %ebx	/* old cs */					; \
	movl %ebx, SYMBOL_NAME(gdb_registers)+40	; \
	popl %ebx	/* old eflags */				; \
	movl %ebx, SYMBOL_NAME(gdb_registers)+36	; \
	movl SYMBOL_NAME(gdb_registers)+16, %ebx	; \
	movl %esp, SYMBOL_NAME(gdb_registers)+16	; \
	movl %ebx, %esp								; \
	pushal										; \
	movl	$##n, %eax							; \
	jmp	exc_ecode_handler
#else /* KERNEL_DEBUG */
#define EXC_ECODE(n) \
.globl SYMBOL_NAME(exc##n)						; \
SYMBOL_NAME_LABEL(exc##n)						; \
	pushal										; \
	movl	$##n, %eax							; \
	jmp	exc_ecode_handler
#endif /* KERNEL_DEBUG */


/* Do we need pushal? */
#ifdef KERNEL_DEBUG
#define INT(n) \
.globl SYMBOL_NAME(h##n)	; \
SYMBOL_NAME_LABEL(h##n)		; \
	movl %esp, SYMBOL_NAME(gdb_registers)+16	/* save esp */		; \
	movl %eax, SYMBOL_NAME(gdb_registers)		/* save eax */		; \
	movl 0(%esp), %eax												; \
	movl %eax, SYMBOL_NAME(gdb_registers)+32	/* save eip */		; \
	movl 4(%esp), %eax												; \
	movl %eax, SYMBOL_NAME(gdb_registers)+40	/* save cs */		; \
	movl 8(%esp), %eax												; \
	movl %eax, SYMBOL_NAME(gdb_registers)+36	/* save eflags */	; \
	movl SYMBOL_NAME(gdb_registers), %eax 		/* restore eax */	; \
    pushal					; \
	movl $##n, %eax			; \
	jmp int_handler
#else /* KERNEL_DEBUG */
#define INT(n) \
.globl SYMBOL_NAME(h##n)	; \
SYMBOL_NAME_LABEL(h##n)		; \
    pushal					; \
	movl $##n, %eax			; \
	jmp int_handler
#endif

.text

EXC_NORMAL(0)
EXC_NORMAL(1)
EXC_NORMAL(2)
EXC_NORMAL(3)
EXC_NORMAL(4)
EXC_NORMAL(5)
EXC_NORMAL(6)
EXC_NORMAL(7)
EXC_ECODE(8)
EXC_NORMAL(9)
EXC_ECODE(10)
EXC_ECODE(11)
EXC_ECODE(12)
EXC_ECODE(13)
EXC_ECODE(14)
EXC_NORMAL(15)
EXC_NORMAL(16)

exc_normal_handler:
	/* push parameter(exception number) and call handler */
	pushl	%eax
	call	*SYMBOL_NAME(esr_table)(, %eax, 4)
	popl	%eax

	popal
	iret

exc_ecode_handler:
	/* push parameter(exception number) and call handler */
	pushl	%eax
	call	*SYMBOL_NAME(esr_table)(, %eax, 4)
	popl	%eax

	popal

	/* pop up error number */
	addl	$4,%esp
	iret


INT(0)
INT(1)
INT(2)
INT(3)
INT(4)
INT(5)
INT(6)
INT(7)
INT(8)
INT(9)
INT(10)
INT(11)
INT(12)
INT(13)
INT(14)
INT(15)

#if 0
int_handler:
	/* push parameter(interrupt number) and call handler */
	pushl	%eax
	call	*SYMBOL_NAME(isr_table)(, %eax, 4)
	popl	%ebx

	/* Send EOI to master & slave (if necessary) PIC */
	movb	$0x20,%al
	cmpl	$0x08,%ebx
	jb	eoi_master
eoi_slave:              
	movl	$0xA0,%edx
	outb	%al,%dx
eoi_master:             
	movl	$0x20,%edx
	outb	%al,%dx

	popal
	iret
#else
int_handler:
	/* push parameter(interrupt number) and call handler */
	pushl	%eax
	popl	%ebx

	/* Send EOI to master & slave (if necessary) PIC */
	movb	$0x20,%al
	cmpl	$0x08,%ebx
	jb	eoi_master
eoi_slave:              
	movl	$0xA0,%edx
	outb	%al,%dx
eoi_master:             
	movl	$0x20,%edx
	outb	%al,%dx

	pushl	%ebx
	call	*SYMBOL_NAME(isr_table)(, %ebx, 4)
	popl	%eax

	popal
	iret
#endif
