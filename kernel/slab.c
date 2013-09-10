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
 *   slab.c                                    0.3.0
 *
 * COMPONENT
 *   Kernel
 *
 * DESCRIPTION
 *   A realization of slab allocator
 *
 * CHANGELOG
 *   AUTHOR         DATE                    NOTES
 *   Bearix         2006-8-20               Version 0.3.0
 *   Bearix         2006-8-21               introduce create and allocate flags,
 *                                                    remove the need of static allocator,
 *                                                    use align to minimal footprint
 *                                                    reduce the size of kmem_bufctl,
 *                                                    introduce kmalloc/kfree.
 *************************************************************************/ 


/* ***************************************************************************
 * slab interface:
 * struct kmem_cache *kmem_cache_create(
 * char *name, size_t size, int align, unsigned long flags, 
 * void (*constructor)(void *, size_t), * void (*destructor)(void *, size_t));
 *	description: create a cache.
 *
 * void *kmem_cache_alloc(struct kmem_cache *cache, int flags);
 *	description: allocate a object from cache.
 *
 * void kmem_cache_free(struct kmem_cache *cache,void *buf);
 *	description: free a object to cache.
 *
 * void kmem_cache_destroy(struct kmem_cache *cache);
 *  description: destroy a cache.
 *
 * In Dooloo, one slab only occupies one page.
 *****************************************************************************/

#include <config.h>

#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <task.h>
#include <list.h>
#include <slab.h>
#include <page.h>
#include <assert.h>
#include <ipc.h>
#include <bsp.h>


struct kmem_cache sys_cache_cache;

struct kmem_cache *malloc_cache[8];

static struct kmem_cache *bufctl_cache;


extern struct zone_struct sys_zone[];

/*
 * init slab subsystem
 */
void slab_init()
{
	struct kmem_cache *cache = &sys_cache_cache;
	uint32_t flags;
	
	/* initialize the cache of kmem_cache */
	sem_initialize(&(cache->sem), 1);
	strcpy(cache->name, "cache");
	cache->flags = CACHE_FLAG_NOREAP|CACHE_FLAG_SMALL|CACHE_FLAG_VALID;
	cache->buf_siz = sizeof(struct kmem_cache);
	//cache->align = 0;
	cache->constructor = NULL;
	cache->destructor = NULL;
	DC_LINKLIST_INIT(&cache->slab_head, prev, next);
	cache->slab_head.free_list = NULL;

	#ifdef MEMORY_DEBUG
	cache->pagecnt = 0;
	cache->err_page = 0;
	cache->err_bufctl = 0;
	#endif /* MEMORY_DEBUG */

	/* create kmem_cache for bufctl_cache */
	flags = CACHE_FLAG_SMALL;
	bufctl_cache = kmem_cache_create("bufctl", sizeof(struct kmem_bufctl), 0, flags, NULL, NULL);
	assert(bufctl_cache);

	/* create kmem_cache for malloc_cache */
	malloc_cache[0] = kmem_cache_create("buf16", 16, 0, flags, NULL, NULL);
	assert(malloc_cache[0]);
	malloc_cache[1] = kmem_cache_create("buf32", 32, 0, flags, NULL, NULL);
	assert(malloc_cache[1]);
	malloc_cache[2] = kmem_cache_create("buf64", 64, 0, flags, NULL, NULL);
	assert(malloc_cache[2]);
	malloc_cache[3] = kmem_cache_create("buf128", 128, 0, flags, NULL, NULL);
	assert(malloc_cache[3]);
	malloc_cache[4] = kmem_cache_create("buf256", 256, 0, flags, NULL, NULL);
	assert(malloc_cache[4]);
	flags = 0;
	malloc_cache[5] = kmem_cache_create("buf512", 512, 0, flags, NULL, NULL);
	assert(malloc_cache[5]);
	malloc_cache[6] = kmem_cache_create("buf1024", 1024, 0, flags, NULL, NULL);
	assert(malloc_cache[6]);
	malloc_cache[7] = kmem_cache_create("buf2048", 2048, 0, flags, NULL, NULL);
	assert(malloc_cache[7]);
}


/*
 * create a cache
 *   name: cache name for debug
 *   size: buffer's size
 *   align: align of buffer
 *   constructor: contructor of buffer
 *   destructor: destructor of buffer
 *   return: kmem_cache or NULL
 */
struct kmem_cache *kmem_cache_create(char *name, size_t size, int align, uint32_t flags, 
	void (*constructor)(void *, size_t), void (*destructor)(void *, size_t))
{
	struct kmem_cache *cache;

	if(name == NULL || size > 2048)
		return NULL;

	cache = (struct kmem_cache *)kmem_cache_alloc(&sys_cache_cache, 0);
	if(cache)
	{
		sem_initialize(&(cache->sem), 1);
		memset(cache->name, 0, KMEM_CACHE_NAME_LEN);
		memcpy(cache->name, name, strlen(name)>KMEM_CACHE_NAME_LEN-1? (KMEM_CACHE_NAME_LEN-1):strlen(name));
		cache->flags = flags|CACHE_FLAG_VALID;
		if(align)
			cache->buf_siz = ALIGN(size, align);
		else
			cache->buf_siz = size;
		cache->constructor = constructor;
		cache->destructor = destructor;
		DC_LINKLIST_INIT(&cache->slab_head, prev, next);
		cache->slab_head.free_list = NULL;

		if(cache->buf_siz <= (PAGE_SIZE>>3))
			cache->flags |= CACHE_FLAG_SMALL;
		else
			cache->flags &= ~CACHE_FLAG_SMALL;

		#ifdef MEMORY_DEBUG
		cache->pagecnt = 0;
		cache->err_page = 0;
		cache->err_bufctl = 0;
		#endif /* MEMORY_DEBUG */
	}

	return cache;
}


/*
 * destroy a cache
 *   cache: kmem_cache
 */
void kmem_cache_destroy(struct kmem_cache *cache)
{
	if((cache == NULL) || ((cache->flags & CACHE_FLAG_VALID) == 0))
		return;

	if(cache->active_objs == 0)
	{
		sem_destroy(&cache->sem);
		kmem_cache_free(&sys_cache_cache, cache);
	}
}


/* grow cache: request page from page allocator */
void kmem_cache_grow(struct kmem_cache *cache, int flags)
{
	int i, page_idx;
	char *ppage;		/* point to page starting address */
	struct zone_struct *zone = NULL;
	struct page_struct *slab;
	struct kmem_bufctl *pbufctl;

	/* allcate a new page */
	ppage = NULL;
	for(i=0; i<SYS_ZONE_NUMBER; i++)
	{
		page_idx = zone_alloc(&sys_zone[i], 0, flags);
		if(page_idx != -1)
		{
			zone = &sys_zone[i];
			ppage = (char *)(zone->base + (page_idx<<12));
			break;
		}
	}

	if(ppage == NULL)
	{
		#ifdef MEMORY_DEBUG
		cache->err_page++;
		#endif
		return;
	}
	
	//kprintf("ppage=0x%08x\n", ppage);
	slab = ADDR_TO_PAGE(zone, ppage);
	slab->flags |= PAGE_FLAGS_SLAB;
	slab->free_list = NULL;
	slab->alloc_list = NULL;
	slab->pcache = cache;
	slab->active_objs = 0;
	slab->total_objs = 0;

	#ifdef MEMORY_DEBUG
	cache->pagecnt ++;
	#endif

	/* no valid slad found, allocate pages for slab and buffers */
	if(cache->flags & CACHE_FLAG_SMALL)
	{
		/*
		 * for small objects which size less than PAGE_SIZE/8
		 * kmem_bufctl is inside page
		 */
		int n;
		char *pbuf;

		/* caculate number of buffer */
		slab->total_objs = (PAGE_SIZE)/(cache->buf_siz+sizeof(struct kmem_bufctl));
		
		/* add bufctl to slab's free list */
		pbufctl = (struct kmem_bufctl *)(ppage + PAGE_SIZE - sizeof(struct kmem_bufctl));
		pbuf = ppage;
		for(n=slab->total_objs; n>0; n--)
		{
			pbufctl->obj = pbuf;

			/* insert allocated kmem_bufctl into free list */
			if(cache->flags & CACHE_FLAG_POISON)
				memset(pbuf, 0xa5, cache->buf_siz);
			if(cache->constructor)
				cache->constructor(pbuf, cache->buf_siz);
			SINGLE_LIST_INSERT(&(slab->free_list), pbufctl);
			pbufctl --;
			pbuf += cache->buf_siz;
		}

		/* insert slab into kmem_cache's slab list */
		DC_LINKLIST_INSERT(&cache->slab_head, prev, next, slab);
		cache->total_objs += slab->total_objs;
	}
	else
	{
		/*
		 * for big objects which size greater than PAGE_SIZE/8 and less than PAGE_SIZE
		 * kmem_bufctl is allocated from bufctl_cache
		 */
		int n, nbuf;
		char *pbuf;

		/* caculate number of buffer */
		nbuf = PAGE_SIZE/cache->buf_siz;

		/* get kmem_bufctl */
		pbuf = ppage;
		for(n=nbuf; n>0; n--)
		{
			pbufctl = (struct kmem_bufctl *)kmem_cache_alloc(bufctl_cache, flags);
			if(pbufctl == NULL)
			{
				#ifdef MEMORY_DEBUG
				kprintf("SLAB ERROR: Can't get bufctl.\n");
				cache->err_bufctl++;
				#endif /* MEMORY_DEBUG */

				/* kmem_butctl partly allocated */
				break;
			}
			else
			{
				pbufctl->obj = pbuf;

				/* insert allocated kmem_bufctl into free list */
				if(cache->flags & CACHE_FLAG_POISON)
					memset(pbuf, 0xa5, cache->buf_siz);
				if(cache->constructor)
					cache->constructor(pbuf, cache->buf_siz);
				SINGLE_LIST_INSERT(&(slab->free_list), pbufctl);

				pbuf += cache->buf_siz;
				slab->total_objs ++;
			}
		}

		if(slab->total_objs == 0)
		{
			/* free allocated page */
			slab->flags &= ~PAGE_FLAGS_SLAB;
			zone_free(zone, page_idx);
		}
		else
		{
			/* insert slab into kmem_cache's slab list */
			DC_LINKLIST_INSERT(&cache->slab_head, prev, next, slab);
			cache->total_objs += slab->total_objs;
		}
	}
}

/*
 * alloc buffer from cache
 *   cache: kmem_cache
 *   flags: can block or not
 *   return: pointer to buffer or NULL
 */
void *kmem_cache_alloc(struct kmem_cache *cache, int flags)
{
	struct page_struct *slab;
	struct kmem_bufctl *pbufctl;
	int ret;

	if((cache == NULL) || ((cache->flags & CACHE_FLAG_VALID) == 0))
		return NULL;

	ret = sem_pend(&cache->sem, flags?TIME_WAIT_NONE:TIME_WAIT_FOREVER);
	if(ret != ROK)
	{
		#ifdef MEMORY_DEBUG
		kprintf("kmem_cache_alloc %s sem_pend() failed, ret=%d\n", cache->name, ret);
		#endif /* MEMORY_DEBUG */
		return NULL;
	}

	/* check whether need to grow cache */
	if((cache->total_objs - cache->active_objs) <= ((cache->flags & 0xff000000) >> 24))
		kmem_cache_grow(cache, flags);

	if(cache->total_objs - cache->active_objs <= 0)
	{
		/* allocation failed */
		sem_post(&cache->sem);
		return NULL;
	}
	
	/* ok, successful, find a valid slab */
	slab = cache->slab_head.next;
	while(slab != &(cache->slab_head))
	{
		if(slab->free_list)
			break;
		else
			slab = slab->next;
	}

	assert(slab != &(cache->slab_head));

	/* remove a buffer from slab and return it */
	SINGLE_LIST_REMOVE(&(slab->free_list), &pbufctl);
	slab->active_objs ++;
	cache->active_objs ++;

	/* for big object, inert it into cache's alloc_list */
	if((cache->flags & CACHE_FLAG_SMALL) == 0)
	{
		pbufctl->next = slab->alloc_list;
		slab->alloc_list = pbufctl;
	}
	
	sem_post(&cache->sem);
	assert(pbufctl);
	return pbufctl->obj;
}


/*
 * free buffer to cache
 *   cache: kmem_cache
 *   buf: buffer
 */
void kmem_cache_free(struct kmem_cache *cache, void *buf)
{
	struct zone_struct *zone;
	struct page_struct *slab;
	struct kmem_bufctl *pbufctl = NULL;
	char *ppage;			/* point to page starting address */
	int ret;

	if((cache == NULL) || ((cache->flags & CACHE_FLAG_VALID) == 0) || (buf == NULL))
		return;
	
	zone = get_zone_by_buf(buf);
	if(zone == NULL)
	{
		#ifdef MEMORY_DEBUG
		kprintf("kmem_cache_free(0x%p): zone no find\n", buf);
		#endif /* MEMORY_DEBUG */
		return;
	}

	slab = ADDR_TO_PAGE(zone, (char *)buf);

	ret = sem_pend(&cache->sem, TIME_WAIT_FOREVER);
	if(ret != ROK)
	{
		#ifdef MEMORY_DEBUG
		kprintf("kmem_cache_alloc %s sem_pend() failed, ret=%d\n", cache->name, ret);
		#endif /* MEMORY_DEBUG */
		return;
	}

	/* get kmem_bufctl */
	if(cache->flags & CACHE_FLAG_SMALL)
	{
		/* small buffer */
		int index;
		
		ppage = (char *)((int)buf & ~0xFFF);
		index = ((char *)buf-ppage)/cache->buf_siz;
		pbufctl = (struct kmem_bufctl *)((ppage+PAGE_SIZE)-(index+1)*sizeof(struct kmem_bufctl));
	}
	else
	{	
		/* big buffer */
		struct kmem_bufctl *prevnode = NULL;
		
		pbufctl = slab->alloc_list;
		while(pbufctl)
		{
			if(pbufctl->obj == buf)
			{
				/* found, remove it from alloc_list */
				if(prevnode == NULL)
				{
					slab->alloc_list = pbufctl->next;
					pbufctl->next = NULL;
				}
				else{
					prevnode->next = pbufctl->next;
					pbufctl->next = NULL;
					break;
				}
			}
			else
			{
				prevnode = pbufctl;
				pbufctl = pbufctl->next;
			}
		}
	}

	if(pbufctl == NULL || (pbufctl->obj != buf) )
	{
		#ifdef MEMORY_DEBUG
		kprintf("kmem_cache_free(0x%p): free invalide buffer\n", buf);
		#endif /* MEMORY_DEBUG */
		return;
	}

	/* insert pbufctl into slab's free_list */
	slab->active_objs --;
	cache->active_objs --;
	SINGLE_LIST_INSERT(&(slab->free_list), pbufctl);

	/* check whether need to reap page */
	if((slab->active_objs == 0) && 
		((cache->total_objs-slab->total_objs - cache->active_objs)>=(cache->flags & 0xff000000) >> 24))
	{
		/* free all buffers in slab */
		if(cache->flags & CACHE_FLAG_SMALL)
		{
			pbufctl = slab->free_list;
			while(pbufctl)
			{
				if(cache->destructor)
					cache->destructor(pbufctl, cache->buf_siz);
				pbufctl = pbufctl->next;
			}
		}
		else
		{
			pbufctl = slab->free_list;
			while(pbufctl)
			{
				if(cache->destructor)
					cache->destructor(pbufctl, cache->buf_siz);
				kmem_cache_free(bufctl_cache, pbufctl);
				pbufctl = pbufctl->next;
			}
		}

		/* remove this page */
		DC_LINKLIST_REMOVE(NULL, prev, next, slab);
		#ifdef MEMORY_DEBUG
		cache->pagecnt --;
		#endif
//kprintf("zone_free(): index=%d\n", slab-zone->page);
		zone_free(zone, slab - zone->page);
	}

	sem_post(&cache->sem);
}


#if 0
static uint8_t sizemap[MAX_PAGES_IN_ZONE-1];
#else
static uint8_t sizemap[] = {
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
};
#endif


void memory_init()
{
#if 0
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
#endif
	page_init();

	slab_init();
}


void *kmalloc(size_t size, int flags)
{
	int order;
	
	if(size<=0)
		return NULL;

	if(size<=2048)
	{
		/* allocate from slab system */
		order = sizemap[(size-1)>>4];	/* slab's minimal unit is 16 bytes */
		return kmem_cache_alloc(malloc_cache[order], flags);
	}
	else
	{
		/* allocate from page system directly */
		int i;
		int page_idx;
		struct zone_struct *zone;
		char *buf = NULL;

		if(size > MAX_PAGES_IN_ZONE<<12)
		{
			#ifdef MEMORY_DEBUG
			kprintf("error: mallloc size is bigger than max available block\n");
			#endif /* MEMORY_DEBUG */
			return NULL;
		}

		/* get order */
		size = (size-1)>>12;
		if(size>=65536)
			order = sizemap[size>>16] + 16;
		else if(size>=256)
			order = sizemap[size>>8] + 8;
		else
			order = sizemap[size];

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


void kfree(void *p)
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


