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
 *   This file defines malloc() and free() api
 *
 * @History
 *   AUTHOR         DATE                 NOTES
 *   Bearix         2006-8-28               use flags, rename malloc/free to kmalloc/kfree
 */

#include <config.h>

#include <sys/types.h>
#include <inttypes.h>
#include <stdio.h>

#include <s_alloc.h>
#include <slab.h>
#include <page.h>
#include <memory.h>
#include <assert.h>
#include <bsp.h>
#include <journal.h>
#include "uprintf.h"
#include "probability.h"

#if MEMORY_ADVANCE
extern struct kmem_cache *malloc_cache[];
extern struct zone_struct sys_zone[];

static uint8_t sizemap[MAX_PAGES_IN_ZONE/2];
#if 0
static uint8_t sizemap = {
	0, 1, 2, 2, 3, 3, 3, 3,    4, 4, 4, 4, 4, 4, 4, 4,		/* 1 (0~15) */
	5, 5, 5, 5, 5, 5, 5, 5,    5, 5, 5, 5, 5, 5, 5, 5,		/* 2 (16~31) */
	6, 6, 6, 6, 6, 6, 6, 6,    6, 6, 6, 6, 6, 6, 6, 6,		/* 3 (32~63) */
	6, 6, 6, 6, 6, 6, 6, 6,    6, 6, 6, 6, 6, 6, 6, 6,		/* 4 */
	7, 7, 7, 7, 7, 7, 7, 7,    7, 7, 7, 7, 7, 7, 7, 7,		/* 5 (64~127) */
	7, 7, 7, 7, 7, 7, 7, 7,    7, 7, 7, 7, 7, 7, 7, 7,		/* 6 */
	7, 7, 7, 7, 7, 7, 7, 7,    7, 7, 7, 7, 7, 7, 7, 7,		/* 7 */
	7, 7, 7, 7, 7, 7, 7, 7,    7, 7, 7, 7, 7, 7, 7, 7,		/* 8 */
	8, 8, 8, 8, 8, 8, 8, 8,    8, 8, 8, 8, 8, 8, 8, 8,		/* 9 (128~255) */
	8, 8, 8, 8, 8, 8, 8, 8,    8, 8, 8, 8, 8, 8, 8, 8,		/* 10 */
	8, 8, 8, 8, 8, 8, 8, 8,    8, 8, 8, 8, 8, 8, 8, 8,		/* 11 */
	8, 8, 8, 8, 8, 8, 8, 8,    8, 8, 8, 8, 8, 8, 8, 8,		/* 12 */
	8, 8, 8, 8, 8, 8, 8, 8,    8, 8, 8, 8, 8, 8, 8, 8,		/* 13 */
	8, 8, 8, 8, 8, 8, 8, 8,    8, 8, 8, 8, 8, 8, 8, 8,		/* 14 */
	8, 8, 8, 8, 8, 8, 8, 8,    8, 8, 8, 8, 8, 8, 8, 8,		/* 15 */
	8, 8, 8, 8, 8, 8, 8, 8,    8, 8, 8, 8, 8, 8, 8, 8,		/* 16 */
	9, 9, 9, 9, 9, 9, 9, 9,    9, 9, 9, 9, 9, 9, 9, 9,		/* 17 (256~511) */
	9, 9, 9, 9, 9, 9, 9, 9,    9, 9, 9, 9, 9, 9, 9, 9,		/* 18 */
	9, 9, 9, 9, 9, 9, 9, 9,    9, 9, 9, 9, 9, 9, 9, 9,		/* 19 */
	9, 9, 9, 9, 9, 9, 9, 9,    9, 9, 9, 9, 9, 9, 9, 9,		/* 20 */
	9, 9, 9, 9, 9, 9, 9, 9,    9, 9, 9, 9, 9, 9, 9, 9,		/* 21 */
	9, 9, 9, 9, 9, 9, 9, 9,    9, 9, 9, 9, 9, 9, 9, 9,		/* 22 */
	9, 9, 9, 9, 9, 9, 9, 9,    9, 9, 9, 9, 9, 9, 9, 9,		/* 23 */
	9, 9, 9, 9, 9, 9, 9, 9,    9, 9, 9, 9, 9, 9, 9, 9,		/* 24 */
	9, 9, 9, 9, 9, 9, 9, 9,    9, 9, 9, 9, 9, 9, 9, 9,		/* 25 */
	9, 9, 9, 9, 9, 9, 9, 9,    9, 9, 9, 9, 9, 9, 9, 9,		/* 26 */
	9, 9, 9, 9, 9, 9, 9, 9,    9, 9, 9, 9, 9, 9, 9, 9,		/* 27 */
	9, 9, 9, 9, 9, 9, 9, 9,    9, 9, 9, 9, 9, 9, 9, 9,		/* 28 */
	9, 9, 9, 9, 9, 9, 9, 9,    9, 9, 9, 9, 9, 9, 9, 9,		/* 29 */
	9, 9, 9, 9, 9, 9, 9, 9,    9, 9, 9, 9, 9, 9, 9, 9,		/* 30 */
	9, 9, 9, 9, 9, 9, 9, 9,    9, 9, 9, 9, 9, 9, 9, 9,		/* 31 */
	9, 9, 9, 9, 9, 9, 9, 9,    9, 9, 9, 9, 9, 9, 9, 9,		/* 32 */
};
#endif


void memory_inita()
{	
	s_alloc_init();
}


void memory_initb()
{
	int i, j;

	/* init sizemap */
	sizemap[0] = 0;
	for(i=0; i<MAX_AREAS_IN_ZONE-1; i++)
	{
		for(j=0; j < (1<<i); j++)
		{
			sizemap[(1<<i)+j] = i+1;
		}
	}

	page_init();

	slab_init();
}


void *malloc(size_t size)
{
	if(size<=0)
		return NULL;

	if(size<=2048)
	{
		/* allocate from slab system */
		return kmem_cache_alloc(malloc_cache[sizemap[(size-1)>>4]], 0);
	}
	else
	{
		/* allocate from page system directly */
		int i, order;
		int page_idx;
		struct zone_struct *zone;
		char *buf = NULL;

		if(size > MAX_PAGES_IN_ZONE<<12)
		{
			kprintf("error: mallloc size is bigger than max available block\n");
			return NULL;
		}

		/* get order */
		order = sizemap[(size-1)>>12];

		for(i=0; i<SYS_ZONE_NUMBER; i++)
		{
			zone = &sys_zone[i];
			page_idx = zone_alloc(zone, order, 0);
			if(page_idx != -1)
			{
				buf = PAGE_TO_ADDR(zone, &(zone->page[page_idx]));
				break;
			}
		}

		return buf;
	}
}


void free(void *p)
{
	struct zone_struct *zone;
	struct page_struct *page;

	/* get zone */
	zone = get_zone_by_buf(p);

	if(zone == NULL)
	{
		kprintf("error: free a buffer not inside any zone\n");
		return;
	}

	/* get page in zone */
	page = ADDR_TO_PAGE(zone, (char *)p);
	
	if(page->flags & PAGE_FLAGS_SLAB)
	{
		/* allocated from slab system */
		kmem_cache_free(page->pcache, p);
	}
	else
	{
		/* allocated from page system */
		zone_free(zone, page - zone->page);
	}
}

#else
/* memory pool */
static uint8_t mem_block_size64[MEMORY_SIZE64_N][MEMORY_SIZE64];
static uint8_t mem_block_size256[MEMORY_SIZE256_N][MEMORY_SIZE256];
static uint8_t mem_block_size768[MEMORY_SIZE768_N][MEMORY_SIZE768];
static uint8_t mem_block_sizebig[MEMORY_SIZEBIG_N][MEMORY_SIZEBIG];

extern S_CACHE s_cache_array[];
void memory_dump()
{
	uint16_t i;
	S_CACHE *sc;
	
	/* dump free memory */
	for(i=0; i<4; i++)
	{
		sc = &s_cache_array[i];
		uprintf(UPRINT_INFO, UPRINT_BLK_MEM, "%s: free %d/%d, base 0x%x, objsize %d\n", 
				sc->name, sc->free_objs, sc->obj_tnum, sc->obj_base, sc->obj_size);
	}
}

void memory_inita()
{	
	s_alloc_init();
}

void memory_initb()
{
	/* index 0 */
	s_cache_create("MEMS64", (S_OBJ *)&mem_block_size64[0], MEMORY_SIZE64, MEMORY_SIZE64_N);
	/* index 1 */
	s_cache_create("MEMS256", (S_OBJ *)&mem_block_size256[0], MEMORY_SIZE256, MEMORY_SIZE256_N);
	/* index 2 */
	s_cache_create("MEMS768", (S_OBJ *)&mem_block_size768[0], MEMORY_SIZE768, MEMORY_SIZE768_N);
	/* index 3 */
	s_cache_create("MEMSBIG", (S_OBJ *)&mem_block_sizebig[0], MEMORY_SIZEBIG, MEMORY_SIZEBIG_N);

	//uprintf_set_enable(UPRINT_INFO, UPRINT_BLK_MEM, 1);
}

void *malloc(size_t size)
{
	void *obj = NULL;
	
	if(size <= MEMORY_SIZE64)
		obj = s_cache_alloc(0);
	else if(size <= MEMORY_SIZE256)
		obj = s_cache_alloc(1);
	else if(size <= MEMORY_SIZE768)
		obj = s_cache_alloc(2);
	else if(size <= MEMORY_SIZEBIG)
		obj = s_cache_alloc(3);

	if(obj == NULL)
		kprintf("error: out of memory task=%s size=%d\n", current->name, size);
	else{
#ifdef INCLUDE_JOURNAL
		journal_memory_alloc(size, obj, NULL, 0, TASK_T(current));
#endif // INCLUDE_JOURNAL
	}

	return obj;
}

void free(void *p)
{
	uint32_t i;
	int found=-1, size = 0;

	for(i=0; i<MEMORY_SIZEBIG_N; i++)
		if(p == &mem_block_sizebig[i])
		{
			found = 3;
			size = MEMORY_SIZEBIG;
			break;
		}


	for(i=0; found<0 && i<MEMORY_SIZE768_N; i++)
		if(p == &mem_block_size768[i])
		{
			found = 2;
			size = MEMORY_SIZE768;
			break;
		}

	for(i=0; found<0 && i<MEMORY_SIZE256_N; i++)
		if(p == &mem_block_size256[i])
		{
			found = 1;
			size = MEMORY_SIZE256;
			break;
		}

	for(i=0; found<0 && i<MEMORY_SIZE64_N; i++)
		if(p == &mem_block_size64[i])
		{
			found = 0;
			size = MEMORY_SIZE64;
			break;
		}
		
	if(found>=0)
	{
		s_cache_free(found, p);
#ifdef INCLUDE_JOURNAL
		journal_memory_free(size, p, NULL, 0, TASK_T(current));
#endif // INCLUDE_JOURNAL
	}
	else
		kprintf("error: invalid pointer for free: addr=0x%x task=%s\n", p, current->name);
}

#endif /* MEMORY_ADVANCE */

