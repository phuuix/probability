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
 *   sprintf for integer type
 *
 * @History
 *   AUTHOR         DATE           NOTES
 */

#ifndef __SIPRINTF_H__
#define __SIPRINTF_H__

#include <stdarg.h>
#include <stddef.h>

typedef struct printdev
{
	void *internal;
	unsigned short limit;
	unsigned short offset;
	int (*outchar)(struct printdev *dev, char c);
}printdev_t;

int doiprintf(printdev_t *dev, const char *format, va_list args );
int siprintf(char *buf, size_t size, char *fmt, ...);
int string_outchar(printdev_t *dev, char c);

#endif /* __SIPRINTF_H__ */
