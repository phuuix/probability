/*************************************************************************/
/* The Dooloo kernel                                                     */
/* Copyright (C) 2004-2006 Xiong Puhui (Bearix)                          */
/* All Rights Reserved.                                                  */
/*                                                                       */
/* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS  */
/* THE PROPERTY OF DOOLOO RTOS DEVELOPMENT TEAM                          */
/*                                                                       */
/*************************************************************************/

/*************************************************************************
 *
 * FILE                                       VERSION
 *   page.c                                    0.3.0
 *
 * COMPONENT
 *   Kernel
 *
 * DESCRIPTION
 *   Page allocator head file
 *
 * CHANGELOG
 *   AUTHOR         DATE                    NOTES
 *   Bearix         2006-8-20               Version 0.3.0
 *************************************************************************/ 


#ifndef __D_PAGE_H__
#define __D_PAGE_H__

#include <config.h>
#include <sys/types.h>
#include <ipc.h>

#define PAGE_SIZE 0x1000

#define PAGE_TO_ADDR(z, p) (char *)(z->base + ((p - z->page)<<12))
#define ADDR_TO_PAGE(z, a) (struct page_struct *)(z->page + ((a - z->base)>>12))

#define PAGE_FLAGS_ALLOC	0x01
#define PAGE_FLAGS_SLAB		0x02


struct kmem_bufctl;
struct kmem_cache;

struct page_struct
{
	struct page_struct *prev;
	struct page_struct *next;
	uint16_t flags;								/* flags */
	uint16_t order;								/* allocation unit (in order), used for page free */

	/* for slab */
	struct kmem_bufctl *free_list;				/* list of free bufctl */
	struct kmem_bufctl *alloc_list;				/* for large buffer, list of allocated bufctl */
	struct kmem_cache *pcache;					/* pointer to kmem_cache */
	
	uint16_t active_objs;							/* active objects number */
	uint16_t total_objs;							/* total objects number */
};

struct area_struct
{
	struct page_struct *prev;
	struct page_struct *next;
#ifdef MEMORY_DEBUG
	uint16_t freecnt;								/* count of freed pages */
	uint16_t alloccnt;								/* count of allocated pages */
#endif
};

struct zone_struct
{
	struct mutex mtx;
	char *base;									/* base address of zone */
	int pagecnt;								/* count of all pages in zone */
	unsigned int bitmap[MAX_PAGES_IN_ZONE/32];	/* page bitmap, MAX_PAGES_IN_ZONE must be doubles of bits of int (32) */
	struct page_struct page[MAX_PAGES_IN_ZONE];	/* page of zone */
	struct area_struct area[MAX_AREAS_IN_ZONE];	/* allocation area */
};


int zone_alloc(struct zone_struct *zone, int order, int flags);
void zone_free(struct zone_struct *zone, int page_idx);
void zone_init(struct zone_struct *zone, void *base, int pagecnt);

void page_init();

struct zone_struct *get_zone_by_page(struct page_struct *page);
struct zone_struct *get_zone_by_buf(char *buf);

#endif /* __D_PAGE_H__ */
