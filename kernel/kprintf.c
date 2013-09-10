/* 
 * kprintf.c
 * none buffered printf, used for kernel debug
 */

/*  kprintf() interface */

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <bsp.h>

#include "siprintf.h"

#define PAD_RIGHT 1
#define PAD_ZERO 2

extern int kprint_outchar(printdev_t *dev, char c);

static printdev_t kprintdev = {NULL, 0, 0, kprint_outchar};
static uint32_t kprintlevel = 0;
//kprintdev.outchar = kprint_outchar;

void kprintf_init(void *internal, uint8_t limit, int (*outchar)(printdev_t *, char), uint32_t trace_level)
{
	kprintdev.internal = internal;
	kprintdev.limit = limit;
	kprintdev.outchar = outchar;
	kprintdev.offset = 0;
	kprintlevel = trace_level;
}

int kprintf(char *fmt,...)
{
	va_list parms;
	int result;
	uint32_t f;

	f = bsp_fsave(); // prevent from interruption
	va_start(parms,fmt);
	result = doiprintf(&kprintdev,fmt,parms);
	va_end(parms);
	bsp_frestore(f);

	return(result);
}



