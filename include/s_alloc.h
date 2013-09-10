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
/*   s_alloc.h                                 0.3.0                     */
/*                                                                       */
/* COMPONENT                                                             */
/*   Kernel                                                              */
/*                                                                       */
/* DESCRIPTION                                                           */
/*   a static alloctor used for kernel                                   */
/*                                                                       */
/* CHANGELOG                                                             */
/*   AUTHOR         DATE                    NOTES                        */
/*   Bearix         2006-8-20               Version 0.3.0                */
/*************************************************************************/ 

#ifndef __D_S_ALLOC_H__
#define __D_S_ALLOC_H__

#include <config.h>

#define S_CACHE_NAME_LEN 8

typedef int s_cache_t;

struct s_obj
{
	struct s_obj *next;
	void *data;	
};

typedef struct s_obj S_OBJ;

struct s_cache
{
	struct s_cache *next;
	char name[S_CACHE_NAME_LEN];		/* name of s_cache */

	void *obj_base;
	uint16_t obj_size;					/* size of object */
	uint16_t obj_tnum;                  /* total number of object */
	uint16_t free_objs;					/* free object number */
	struct s_obj *free_obj_list;		/* free object list */
};
typedef struct s_cache S_CACHE;

void s_alloc_init();
s_cache_t s_cache_create(char *name, S_OBJ *sarray, size_t ssize, size_t snumber);
void s_cache_destroy(s_cache_t s);
void *s_cache_alloc(s_cache_t s);
void s_cache_free(s_cache_t s, void *pobj);

#endif /* __D_S_ALLOC_H__ */
