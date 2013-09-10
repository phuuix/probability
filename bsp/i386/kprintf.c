/* 
 * kprintf.c
 */

/*  Console output functions    */

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

int kprintf(char *fmt,...)
{
	char cbuf[200];
	va_list parms;
	int result;

	va_start(parms,fmt);
	result = vsnprintf(cbuf,200,fmt,parms);
	va_end(parms);
	cputs(cbuf);
	return(result);
}

