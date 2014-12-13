/*
 *
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
 *   Assert & demand: diagnostic routine .
 *
 * @History
 *   AUTHOR         DATE           NOTES
 */


#ifndef __D_ASSERT_H__
#define __D_ASSERT_H__

#include <bsp.h>

#define assert(x) {														\
  if (! (x))															\
  {																		\
    kprintf("%s:%d: assertion failed: %s\n", __FILE__, __LINE__, #x);   \
    dump_call_stack(get_fp(), MEMLOW, MEMHIGH);                         \
	panic("assert failure\n");											\
  }																		\
}


#endif /* __D_ASSERT_H__ */
