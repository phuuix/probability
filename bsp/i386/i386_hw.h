/* i386_hw.h */

#ifndef __D_I386_HW_H__
#define __D_I386_HW_H__

#include <sys/types.h>

/* compatible with Linux */
static __inline__ void outb( unsigned char val, unsigned short int port )
{
	__asm__ volatile ("outb %%al,%%dx"::"a" (val),"d" (port));
}

static __inline__ unsigned char inb( unsigned short int port )
{
	unsigned char ret; 
	__asm__ volatile ("inb %%dx,%%al":"=a" (ret):"d" (port)); 
	return ret;
}

static __inline__ void outw( unsigned int val, unsigned short int port )
{
	__asm__ volatile ("outw %%ax,%%dx"::"a" (val),"d" (port));
}

static __inline__ unsigned int inw( unsigned short int port )
{
	unsigned int ret; 
	__asm__ volatile ("inw %%dx,%%ax":"=a" (ret):"d" (port));
	return ret;
}

static __inline__ void outl( unsigned int val, unsigned short int port )
{
	__asm__ __volatile__("outl %0,%1" : :"a" (val), "d" (port));
}

static __inline__ unsigned int inl( unsigned short int port )
{
	unsigned int ret;
	__asm__ __volatile__("inl %1,%0" : "=a" (ret) : "d" (port));
	return ret;
}

/*
#define inb inp
#define inw inpw
#define inl inpd
#define outb outp
#define outw outpw
#define outl outpd
*/

inline unsigned char inp(unsigned short _port);

inline unsigned short inpw (unsigned short _port);

inline unsigned long inpd(unsigned short _port);

inline void outp(unsigned short _port, unsigned char _data);

inline void outpw(unsigned short _port, unsigned short _data);

inline void outpd(unsigned short _port, unsigned long _data);

inline WORD get_CS(void);

inline WORD get_DS(void);

inline WORD get_FS(void);

inline DWORD get_SP(void);

inline DWORD get_BP(void);

inline WORD get_TR(void);

inline void set_TR(WORD n);

inline void set_LDTR(WORD addr);


/* Clear Task Switched Flag! Used for FPU preemtion */
inline void clts(void);

/* Halt the processor! */
inline void halt(void);

/* These functions are used to mask/unmask interrupts           */
inline void sti(void);
inline void cli(void);

/*
inline DWORD fsave(void);

inline void frestore(DWORD f);
*/

void IDT_place(BYTE num,void (*handler)(void));

void set_TSS(BYTE num,DWORD base,DWORD lim);

void GDT_place(BYTE num,DWORD base,DWORD lim,BYTE acc,BYTE gran);

extern inline int test_and_set(unsigned char *addr)
{
   unsigned char result = 1;
   asm ("xchgb %1, %0":"=m" (*addr),"=r" (result)
                      :"1" (result) :"memory");
   return result;
}

extern inline void lmempokeb(void* a, BYTE v)
{
	*((BYTE *)a) = v;
}
extern inline void lmempokew(void* a, WORD v)
{
	*((WORD *)a) = v;
}
extern inline void lmempoked(void* a, DWORD v)
{
	*((DWORD *)a) = v;
}

extern inline BYTE lmempeekb(void* a)
{
	return *((BYTE *)a);
}
extern inline WORD lmempeekw(void* a)
{
	return *((WORD *)a);
}
extern inline DWORD lmempeekd(void* a)
{
	return *((DWORD *)a);
}

#endif /* __D_I386_HW_H__ */

