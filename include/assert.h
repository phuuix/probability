/*************************************************************************/
/* The Dooloo kernel                                                     */
/* Copyright (C) 2004-2006 Xiong Puhui (Bearix)                          */
/* All Rights Reserved.                                                  */
/*                                                                       */
/* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS  */
/* THE PROPERTY OF DOOLOO RTOS DEVELOPMENT TEAM                          */
/*                                                                       */
/*************************************************************************/

/*************************************************************************/
/*                                                                       */
/* FILE                                       VERSION                    */
/*   assert.h                                  0.3.0                     */
/*                                                                       */
/* COMPONENT                                                             */
/*   Kernel                                                              */
/*                                                                       */
/* DESCRIPTION                                                           */
/*   Assert & demand: diagnostic routine                                 */
/*                                                                       */
/* CHANGELOG                                                             */
/*   AUTHOR         DATE                    NOTES                        */
/*   Bearix         2006-8-20               Version 0.3.0                */
/*************************************************************************/ 


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
