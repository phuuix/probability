/* i386_stub.h */

#ifndef __D_I386_STUB_H__
#define __D_I386_STUB_H__

#define GDB_PRINTF kprintf

#define BREAKPOINT() asm("   int $3");

/* Number of bytes of registers.  */
#define NUMREGBYTES 64

/* number of zbreaks */
#define MAX_ZBREAK_NUMBER 32

enum regnames {EAX, ECX, EDX, EBX, ESP, EBP, ESI, EDI,
	       PC /* also known as eip */,
	       PS /* also known as eflags */,
	       CS, SS, DS, ES, FS, GS};

struct zbreak
{
	struct zbreak *prev;
	struct zbreak *next;

	unsigned long address;
	unsigned char buffer[4];
};

//extern struct zbreak valid_zbreak_list, free_zbreak_list;

void gdb_stub_init();
void breakpoint();
void gdb_init_udp();
void gdb_udp_poll_start();
void gdb_udp_poll_stop();
void gdb_udp_ipaddr_set(uint32_t ip);
uint32_t gdb_udp_ipaddr_get();

#endif /* __D_I386_STUB_H__ */

