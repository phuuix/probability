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
 *   a static alloctor used for kernel
 *
 * @History
 *   AUTHOR         DATE           NOTES
 */

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
