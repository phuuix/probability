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
/*   memory_cmd.c                                    0.3.0               */
/*                                                                       */
/* COMPONENT                                                             */
/*   Shell                                                               */
/*                                                                       */
/* DESCRIPTION                                                           */
/*                                                                       */
/*                                                                       */
/* CHANGELOG                                                             */
/*   AUTHOR         DATE                    NOTES                        */
/*   Bearix         2006-8-20               Version 0.3.0                */
/*************************************************************************/ 

#include <config.h>

#include <stdio.h>
#include <string.h>
#include <sys/types.h>

#include <page.h>
#include <slab.h>
#include <shell.h>
#include <task.h>

extern struct kmem_cache *malloc_cache[];
extern struct zone_struct sys_zone[];

int show_slab(struct shell_session *ss, int argc, char **argv)
{
	int n, i;
	struct page_struct *slab;
	struct kmem_cache *cache;

	for(n=0; n<8; n++)
	{
		cache = malloc_cache[n];

		ss->output("cache %s: pagecnt=%d err_page=%d err_bufctl=%d\n", cache->name,
				cache->pagecnt, cache->err_page, cache->err_bufctl);

		task_lock();
		i = 1;
		slab = cache->slab_head.next;
		while(slab != &(cache->slab_head))
		{
			ss->output("slab%d flages(%04x) active_objs(%d) totel_objs(%d)\n", i++, 
					slab->flags, slab->active_objs, slab->total_objs);
			slab = slab->next;
		}
		task_unlock();
	}
	

	return SHELL_RET_OK;
}


int show_page(struct shell_session *ss, int argc, char **argv)
{
	struct zone_struct *zone;
	int i;

	zone = &(sys_zone[0]);
	ss->output("base: %08x pagecnt: %d\n", zone->base, zone->pagecnt);
	ss->output("bitmap: \n");
	for(i=0; i<zone->pagecnt/32; i++)
	{
		if(i%4 == 0)
			ss->output("\n%2d: ", i);
		ss->output("%08x ", zone->bitmap[i]);
	}

	ss->output("\n\narea freecnt alloccnt\n");
	for(i=0; i<MAX_AREAS_IN_ZONE; i++)
	{
		ss->output("%d:     %d       %d\n", i, zone->area[i].freecnt, zone->area[i].alloccnt);
	}

	return SHELL_RET_OK;
}
