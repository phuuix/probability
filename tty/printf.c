/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
 *
 * Copyright (c) Puhui Xiong <phuuix@163.com>
 * @file
 *   printf implementation
 *
 * @History
 *   AUTHOR         DATE                 NOTES
 *   puhuix           
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




