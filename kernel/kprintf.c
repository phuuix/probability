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
 *   none buffered printf, used for kernel debug
 *
 * @History
 *   AUTHOR         DATE           NOTES
 */

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

	SYS_FSAVE(f); // prevent from interruption
	va_start(parms,fmt);
	result = doiprintf(&kprintdev,fmt,parms);
	va_end(parms);
	SYS_FRESTORE(f);

	return(result);
}



