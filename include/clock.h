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
 *   clock routines
 *
 * @History
 *   AUTHOR         DATE           NOTES
 */


#ifndef __D_CLOCK_H__
#define __D_CLOCK_H__
#include <sys/types.h>

uint32_t tick();
uint32_t time(uint32_t *t);
void clock_init(int irq);

#endif /* __D_CLOCK_H__ */
