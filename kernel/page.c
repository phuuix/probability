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
 *   page allocation arithmetic: Buddy system
 *
 * CHANGELOG
 *   AUTHOR         DATE                    NOTES
 *   Bearix         2006-8-20               Version 0.3.0
 *   Bearix         2006-9-03               alloc page with no wait
 *************************************************************************/ 


#include <config.h>

#include <stdlib.h>
#include <string.h>
#include <task.h>
#include <ipc.h>
#include <page.h>
#include <bsp.h>
#include <list.h>

struct zone_struct sys_zone[SYS_ZONE_NUMBER];

static unsigned int bitmap_const[] = {
	0x80000000,		/* 1 pages */
	0xc0000000,		/* 2 pages */
	0xf0000000,		/* 4 pages */
	0xff000000,		/* 8 pages */
	0xffff0000,		/* 16 pages */
	0xffffffff,		/* 32 pages */
	0xffffffff,		/* 64 pages */
	0xffffffff,		/* 128 pages */
	0xffffffff,		/* 256 pages */
	0xffffffff,		/* 512 pages */
	0xffffffff,		/* 1024 pages */
	0xffffffff,		/* 2048 pages */
	0xffffffff,		/* 4096 pages */
	0xffffffff,		/* 8192 pages */
};


/*
 * set zone's bitmap: start from page
 *   zone: pointer to zone
 *   page: start position idicated by page
 *   order: set bits equal to 1<<order
 */
static inline void set_bitmap(struct zone_struct *zone, struct page_struct *page, int order)
{
	int page_index;
	int bitmap_lw;
	unsigned int bitmap_value;
	int i;

	page_index = page - zone->page;
	bitmap_lw = (1<<order) >> 5;		/* divide by bits of int (32) */
	bitmap_value = bitmap_const[order];

//	kprintf("page_index=%d bitmap_lw=%d bitmap_value=0x%08x\n", page_index, bitmap_lw, bitmap_value);
	if(bitmap_lw == 0)
	{
		bitmap_lw = 1;
		bitmap_value = bitmap_value >> (page_index & (31));
	}

//	kprintf("page_index=%d bitmap_lw=%d bitmap_value=0x%08x\n", page_index, bitmap_lw, bitmap_value);

	for(i=0; i<bitmap_lw; i++)
	{
		zone->bitmap[(page_index>>5)+i] |= bitmap_value;
	}
}


/*
 * clear zone's bitmap: start from page
 *   zone: pointer to zone
 *   page: start position idicated by page
 *   order: set bits equal to 1<<order
 */
static inline void clear_bitmap(struct zone_struct *zone, struct page_struct *page, int order)
{
	int page_index;
	int bitmap_lw;
	unsigned int bitmap_value;
	int i;

	page_index = page - zone->page;
	bitmap_lw = (1<<order) >> 5;
	bitmap_value = bitmap_const[order];

	if(bitmap_lw == 0)
	{
		bitmap_lw = 1;
		bitmap_value = bitmap_value >> (page_index & (31));
	}

	/* clear bitmap */
	for(i=0; i<bitmap_lw; i++)
	{
		zone->bitmap[(page_index>>5)+i] &= ~bitmap_value;
	}
}


/*
 * test whether block is idle
 *   zone: pointer to zone
 *   page: page to be tested
 *   order: order of page
 *   return: 0 if block is idle, 1 if block is bussy
 */
static inline int test_bitmap(struct zone_struct *zone, struct page_struct *page, int order)
{
	int page_index;
	int bitmap_lw;
	unsigned int bitmap_value;
	int i;

	page_index = page - zone->page;
	bitmap_lw = (1<<order) >> 5;
	bitmap_value = bitmap_const[order];

	if(bitmap_lw == 0)
	{
		bitmap_lw = 1;
		bitmap_value = bitmap_value >> (page_index & (31));
	}

	for(i=0; i<bitmap_lw; i++)
	{
		if(zone->bitmap[(page_index>>5)+i] & bitmap_value)
		{
			/* buddy is busy */
			return 1;
		}
	}

	return 0;
}

/*
 * alloc pages from zone
 *   zone: pointer to zone
 *   order: order of area (count of page)
 *   flags: wait or no wait, 0 for wait
 *   return: page index, -1 = error
 */
int zone_alloc(struct zone_struct *zone, int order, int flags)
{
	int cur_order;
	struct area_struct *area;
	struct page_struct *page;
	int page_idx = -1;
	int ret;

	if(zone == NULL || order >= MAX_AREAS_IN_ZONE || order < 0)
	{
		kprintf("zone_alloc() error\n");
		return -1;
	}

	ret = mtx_pend(&zone->mtx, flags?TIME_WAIT_NONE:TIME_WAIT_FOREVER);
	if(ret != ROK) return -1;

	for(cur_order=order; cur_order<MAX_AREAS_IN_ZONE; cur_order++)
	{
		area = &(zone->area[cur_order]);
		if(area->next != (struct page_struct *)area)
		{
//			kprintf("allocate from area %d for order %d\n", cur_order, order);

			page = area->next;
			DC_LINKLIST_REMOVE(NULL, prev, next, page);

			#ifdef MEMORY_DEBUG
			zone->area[cur_order].freecnt --;
			zone->area[order].alloccnt ++;
			#endif

			/* get page index and set page's order */
			page->flags = PAGE_FLAGS_ALLOC;
			page->order = order;
			page_idx = page - zone->page;

			/* set bitmap */
			set_bitmap(zone, page, order);

			/* insert the remained blocks into area */
			if(cur_order != order)
			{
//				kprintf("split area %d\n", cur_order);
				page = page + (1<<order);
				for(; order<cur_order; order++)
				{
					area = &(zone->area[order]);
					DC_LINKLIST_APPEND(((struct page_struct *)area), prev, next, page);
					page = page + (1<<order);

					#ifdef MEMORY_DEBUG
					zone->area[order].freecnt ++;
					#endif
				}
			}

			break;
		}
	}

	mtx_post(&zone->mtx);

	return page_idx;
}


/*
 * free pages to zone
 *   zone: pointer to zone
 *   page_idx: page index
 */
void zone_free(struct zone_struct *zone, int page_idx)
{
	int order;
	struct area_struct *area;
	struct page_struct *buddy;
	int mask;

	if(zone == NULL || page_idx < 0 || page_idx >= zone->pagecnt)
	{
		kprintf("zone_free() error\n");
		return;
	}

	if(!(zone->page[page_idx].flags & PAGE_FLAGS_ALLOC))
	{
		kprintf("zone_free(): invalid page index\n");
		return;
	}
	order = zone->page[page_idx].order;

	zone->page[page_idx].flags = 0;		/* set this page unallcated */
	mask = (~0UL) << order;
	area = &(zone->area[order]);

	mtx_pend(&zone->mtx, TIME_WAIT_FOREVER);

	#ifdef MEMORY_DEBUG
	zone->area[order].alloccnt --;
	#endif
	
	/* clear bitmap */
	clear_bitmap(zone, &(zone->page[page_idx]), order);

	for(; order<MAX_AREAS_IN_ZONE; order++)
	{
//		kprintf("zone_free() order=%d mask=0x%08x\n", order, mask);

		/* get buddy */
		buddy = zone->page + (page_idx ^ -mask);

		/* check whether buddy is busy */
		if(order == MAX_AREAS_IN_ZONE-1 || test_bitmap(zone, buddy, order))
		{
			break;
		}
		
//		kprintf("buddy is idle, order=%d\n", order);

		/* remove buddy from area */
		DC_LINKLIST_REMOVE(NULL, prev, next, buddy);

		#ifdef MEMORY_DEBUG
		zone->area[order].freecnt --;
		#endif

		/* move buddy up one level */
		mask <<= 1;
		area++;
		page_idx &= mask;
	}

	/* insert the free block into it's area */
	DC_LINKLIST_APPEND(((struct page_struct *)area), prev, next, &(zone->page[page_idx]));

	#ifdef MEMORY_DEBUG
	zone->area[order].freecnt ++;
	#endif

	mtx_post(&zone->mtx);
}


/*
 * init a zone
 *   zone: pointer to zone
 *   base: base address of zone
 *   pagecnt: page count
 */
void zone_init(struct zone_struct *zone, void *base, int pagecnt)
{
	int order, pageleft;
	struct area_struct *area;
	struct page_struct *page;

	if(zone == NULL || pagecnt <= 0)
		return;

	memset(zone, 0, sizeof(struct zone_struct));

	zone->base = base;
	zone->pagecnt = pagecnt;
	mtx_initialize(&zone->mtx);

	/* mark unavilable pages busy */
	memset(&zone->bitmap[zone->pagecnt/32], 0xff, (MAX_PAGES_IN_ZONE - zone->pagecnt)/8);

	mtx_pend(&zone->mtx, TIME_WAIT_FOREVER);

	/* init area */
	pageleft = pagecnt;
	page = &(zone->page[0]);
	for(order=MAX_AREAS_IN_ZONE-1; order>=0; order--)
	{
		int blocksiz;
//kprintf("order=%d\n", order);
		area = &(zone->area[order]);
		DC_LINKLIST_INIT(((struct page_struct *)area), prev, next);
		blocksiz = 1 << order;

		while(pageleft >= blocksiz)
		{
			DC_LINKLIST_APPEND(((struct page_struct *)area), prev, next, page);
			page += blocksiz;
			pageleft -= blocksiz;

			#ifdef MEMORY_DEBUG
			area->freecnt ++;
			#endif
//			kprintf("freecnt=%d\n", area->freecnt);
		}
	}

	mtx_post(&zone->mtx);
}


/*
 * page sub-system init
 */
void page_init()
{
	zone_init(&(sys_zone[0]), (char *)ZONE0_MEMORY_LOW_ADDR, ZONE0_PAGE_COUNT);
}


/*
 * return zone structure by page struct
 *   page: page struct
 *   return: zone struct
 */
struct zone_struct *get_zone_by_page(struct page_struct *page)
{
	int i;

	for(i=0; i<SYS_ZONE_NUMBER; i++)
	{
		if(page >= sys_zone[i].page && page < sys_zone[i].page + sys_zone[i].pagecnt)
			return &(sys_zone[i]);
	}

	return NULL;
}


/*
 * return zone structure by buffer
 *   buf: buffer
 *   return: zone struct
 */
struct zone_struct *get_zone_by_buf(char *buf)
{
	int i;

	for(i=0; i<SYS_ZONE_NUMBER; i++)
	{
		if(buf >= sys_zone[i].base && buf < sys_zone[i].base + (sys_zone[i].pagecnt<<12))
			return &(sys_zone[i]);
	}

	return NULL;
}
