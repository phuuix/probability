/*
 * printf.c
 */
#include <stdio.h>
#include <string.h>
#include <tty.h>

#include "siprintf.h"

/*
 * print message
 *   fmt: print format
 */
int printf(const char *fmt, ...)
{
	va_list parms;
	int result = 0;
	char buffer[100];
	printdev_t sprintdev;

	buffer[0] = 0;		/* to first tty */
	sprintdev.internal = buffer+1;
	sprintdev.limit = 98;
	sprintdev.offset = 0;
	sprintdev.outchar = string_outchar;
	
	va_start(parms,fmt);
	result = doiprintf(&sprintdev,fmt,parms);
	va_end(parms);
	
	/* terminate the string */
	buffer[result+1] = '\0';
	
	tty_fw_write(&systty[0], (unsigned char *)(buffer+1), strlen(buffer+1));
	
	return result;
}




