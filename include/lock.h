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
 *   
 *
 * @History
 *   AUTHOR         DATE           NOTES
 */
 

#ifndef __D_LOCK_H__
#define __D_LOCK_H__

#include <sys/types.h>

typedef unsigned char lock_t;

uint8_t atomic_lock(lock_t *l);
void atomic_unlock(lock_t *l);

#endif /* __D_LOCK_H__ */
