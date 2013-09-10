/*
 * i386_hw.c
 */
 
#include "i386_hw.h"
#include "i386_data.h"
#include "i386_sel.h"
#include "i386_tasksw.h"

extern GATE IDT[256];
extern union gdt_entry GDT[256];

inline unsigned char inp(unsigned short _port)
{
    unsigned char rv;
    __asm__ __volatile__ ("inb %1, %0"
	  : "=a" (rv)
	  : "d" (_port));
    return(rv);
}

inline unsigned short inpw (unsigned short _port)
{
    unsigned short rv;
    __asm__ __volatile__ ("inw %1, %0"
	  : "=a" (rv)
	  : "d" (_port));
    return(rv);
}

inline unsigned long inpd(unsigned short _port)
{
    unsigned long rv;
    __asm__ __volatile__ ("inl %1, %0"
	  : "=a" (rv)
	  : "d" (_port));
    return(rv);
}

inline void outp(unsigned short _port, unsigned char _data)
{
    __asm__ __volatile__ ("outb %1, %0"
	  :
	  : "d" (_port),
	    "a" (_data));
}

inline void outpw(unsigned short _port, unsigned short _data)
{
    __asm__ __volatile__ ("outw %1, %0"
	  :
	  : "d" (_port),
	    "a" (_data));
}

inline void outpd(unsigned short _port, unsigned long _data)
{
    __asm__ __volatile__ ("outl %1, %0"
	  :
	  : "d" (_port),
	    "a" (_data));
}

inline WORD get_CS(void)
{WORD r; __asm__ __volatile__ ("movw %%cs,%0" : "=q" (r)); return(r);}

inline WORD get_DS(void)
{WORD r; __asm__ __volatile__ ("movw %%ds,%0" : "=q" (r)); return(r);}

inline WORD get_FS(void)
{WORD r; __asm__ __volatile__ ("movw %%fs,%0" : "=q" (r)); return(r);}

inline DWORD get_SP(void)
{
    DWORD rv;
    __asm__ __volatile__ ("movl %%esp, %0"
	  : "=a" (rv));
    return(rv);
}

inline DWORD get_BP(void)
{
    DWORD rv;
    __asm__ __volatile__ ("movl %%ebp, %0"
	  : "=a" (rv));
    return(rv);
}

inline WORD get_TR(void)
{WORD r; __asm__ __volatile__ ("strw %0" : "=q" (r)); return(r); }

inline void set_TR(WORD n)
{__asm__ __volatile__("ltr %%ax": /* no output */ :"a" (n)); }

inline void set_LDTR(WORD addr)
{ __asm__ __volatile__("lldt %%ax": /* no output */ :"a" (addr)); }

/* Clear Task Switched Flag! Used for FPU preemtion */
inline void clts(void)
{__asm__ __volatile__ ("clts"); }

/* Halt the processor! */
inline void halt(void)
{__asm__ __volatile__ ("hlt"); }

/* These functions are used to mask/unmask interrupts           */
inline void sti(void) {__asm__ __volatile__ ("sti"); }
inline void cli(void) {__asm__ __volatile__ ("cli"); }

inline DWORD bsp_fsave(void)
{
    DWORD result;
    
    __asm__ __volatile__ ("pushfl");
    __asm__ __volatile__ ("cli");
    __asm__ __volatile__ ("popl %eax");
    __asm__ __volatile__ ("movl %%eax,%0"
	: "=r" (result)
	:
	: "eax" );
    return(result);
}

inline void bsp_frestore(DWORD f)
{
    __asm__ __volatile__ ("mov %0,%%eax"
	:
	: "r" (f)
	: "eax");
    __asm__ __volatile__ ("pushl %eax");
    __asm__ __volatile__ ("popfl");
}

/* set IDT table */
void IDT_place(BYTE num,void (*handler)(void))
{
    DWORD offset = (DWORD)(handler);
    IDT[num].sel = X_FLATCODE_SEL;
	IDT[num].access = INT_GATE386;
    IDT[num].dword_cnt = 0;
    IDT[num].offset_lo = offset & 0xFFFF;
    offset >>= 16;
    IDT[num].offset_hi = offset & 0xFFFF;
}

/* set TSS in GDT */
void set_TSS(BYTE num,DWORD base,DWORD lim)
{
	GDT[TSS_BASE+num].d.lim_lo = (lim & 0xFFFF);
	GDT[TSS_BASE+num].d.base_lo = (base & 0x0000FFFF);
	GDT[TSS_BASE+num].d.base_med = ((base & 0x00FF0000) >> 16);
	GDT[TSS_BASE+num].d.base_hi = ((base & 0xFF000000) >> 24);
	GDT[TSS_BASE+num].d.access = FREE_TSS386;
	GDT[TSS_BASE+num].d.gran = 0x0;
}

/* set GDT table */
void GDT_place(BYTE num,DWORD base,DWORD lim,BYTE acc,BYTE gran)
{
    GDT[num].d.base_lo = (base & 0x0000FFFF);
    GDT[num].d.base_med = ((base & 0x00FF0000) >> 16);
    GDT[num].d.base_hi = ((base & 0xFF000000) >> 24);
    GDT[num].d.access = acc;
    GDT[num].d.lim_lo = (lim & 0xFFFF);
//	GDT[num].d.gran = (gran | ((lim >> 16) & 0x0F));
    GDT[num].d.gran = (gran | ((lim >> 16) & 0x0F) | 0x40);
}
