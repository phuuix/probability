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
 *   static allocator
 *
 * @History
 *   AUTHOR         DATE                 NOTES
 *   puhuix           
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <task.h>
#include <config.h>
#include <assert.h>

#include <sys/types.h>
#include <list.h>
#include <s_alloc.h>
#include <bsp.h>

S_CACHE s_cache_array[MAX_S_CACHE_NUMBER];
S_CACHE *s_cache_list;


/*
 * init s_alloc subsystem
 */
void s_alloc_init()
{
	int i;
	
	s_cache_list = NULL;
	for(i=MAX_S_CACHE_NUMBER-1; i>=0; i--)
	{
		SINGLE_LIST_INSERT(&s_cache_list, &(s_cache_array[i]));
	}
}


/*
 * create a s_cache
 *   name: name of s_cache
 *   sarray: array of S_OBJ
 *   ssize: size of S_OBJ
 *   snumber: number of S_OBJ
 *   return: s_cache or -1
 */
s_cache_t s_cache_create(char *name, S_OBJ *sarray, size_t ssize, size_t snumber)
{
	S_CACHE *sc;

	task_lock();
	SINGLE_LIST_REMOVE(&s_cache_list, &sc);
	task_unlock();

	if(sc && sarray && ssize && snumber)
	{
		int i;
		S_OBJ *obj;

		if(name)
			strncpy(sc->name, name, S_CACHE_NAME_LEN);
		else
			memset(sc->name, 0, S_CACHE_NAME_LEN);

		sc->obj_base = sarray;
		sc->obj_size = ssize;
		sc->obj_tnum = snumber;
		sc->free_objs = snumber;
		sc->free_obj_list = NULL;

		/* setup free_obj_list */
		task_lock();
		obj = sarray;
		for(i=0; i<snumber; i++)
		{
			SINGLE_LIST_INSERT(&(sc->free_obj_list), obj);
			obj = (S_OBJ *)((char *)obj+ssize);
		}
		task_unlock();

		return sc-s_cache_array;
	}
	return -1;
}


/*
 * destroy s_cache
 *   s: s_cache
 */
void s_cache_destroy(s_cache_t s)
{
	if(s>=0 && s<MAX_S_CACHE_NUMBER)
	{
		S_CACHE *sc = &s_cache_array[s];

		/* insert s_cache back to s_cache_list */
		task_lock();
		SINGLE_LIST_INSERT(&s_cache_list, sc);
		task_unlock();
	}
}


/*
 * allocate object from s_cache
 *   s: s_cache
 *   return: object or NULL
 */
void *s_cache_alloc(s_cache_t s)
{
	S_OBJ *obj = NULL;
	uint32_t f;

	f = bsp_fsave();
	if(s>=0 && s<MAX_S_CACHE_NUMBER)
	{
		S_CACHE *sc = &s_cache_array[s];

		if(sc->free_objs > 0)
		{
			SINGLE_LIST_REMOVE(&(sc->free_obj_list), &obj);
			sc->free_objs --;
		}
	}
	bsp_frestore(f);
	return (void *)obj;
}


/*
 * free obj to s_cache
 *   s: s_cache
 *   pobj: object
 */
void s_cache_free(s_cache_t s, void *pobj)
{
	uint32_t f;

	f = bsp_fsave();
	if(s>=0 && s<MAX_S_CACHE_NUMBER && pobj)
	{
		S_CACHE *sc = &s_cache_array[s];
		S_OBJ *obj;

		obj = (S_OBJ *)pobj;
		assert((pobj >= sc->obj_base) && (pobj <= sc->obj_base+sc->obj_size*(sc->obj_tnum-1)));
		
		SINGLE_LIST_INSERT(&(sc->free_obj_list), obj);
		sc->free_objs ++;
	}
	bsp_frestore(f);
}


