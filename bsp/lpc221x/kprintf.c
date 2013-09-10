/* 
 * kprintf.c
 */

/*  kprintf() interface */

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <bsp.h>

#include "serial.h"
#include "siprintf.h"

#define PAD_RIGHT 1
#define PAD_ZERO 2

int serial_outchar(printdev_t *dev, char c)
{
	serial_putc(c);
	return 1;
}

int kprintf(char *fmt,...)
{
	printdev_t kprintdev;
	va_list parms;
	int result;
	uint32_t f;

	kprintdev.outchar = serial_outchar;

	f = bsp_fsave(); // prevent from interruption
	va_start(parms,fmt);
	result = doiprintf(&kprintdev,fmt,parms);
	va_end(parms);
	bsp_frestore(f);

	return(result);
}



