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
 *   slab.h                                    0.3.0
 *
 * COMPONENT
 *   Kernel
 *
 * DESCRIPTION
 *   slab memory allocator
 *
 * CHANGELOG
 *   AUTHOR         DATE                    NOTES
 *   Bearix         2006-8-20               Version 0.3.0
 *************************************************************************/ 


#ifndef __D_SLAB_H__
#define __D_SLAB_H__

#include <config.h>
#include <sys/types.h>
#include <ipc.h>
#include <page.h>


#define KMEM_CACHE_NAME_LEN	8
#define SMALL_BUFCTL_SIZE	sizeof(struct kmem_bufctl)

/* flags for kmem_cache_create */
#define CACHE_FLAG_VALID	0x01		/* cache is in use */
#define CACHE_FLAG_SMALL	0x02		/* cache for small object (<512) */
#define CACHE_FLAG_NOREAP	0x04		/* free pages is never reapped */
#define CACHE_FLAG_POISON	0x08		/* Poison the slab with a known test pattern (a5a5a5a5)
                                                                           to catch references to uninitialised memory. */

/* cache auto grow factors: 0, 1, 2, 4, 8, 16, occupy the last 5 bits of flags */


/* flags for kmem_cache_alloc */
#define CACHE_FLAG_NOWAIT	0x1			/* this flag is used to take semaphore */

/*
 * Align VAL to ALIGN, which must be a power of two.
 */
#define ALIGN(val,align)	(((val) + ((align) - 1)) & ~((align) - 1))

#if 0
struct kmem_slab
{
	struct kmem_slab *prev;
	struct kmem_slab *next;

	struct kmem_bufctl *free_list;		/* list of free bufctl */
	struct kmem_bufctl *hash;			/* for large buffer, buffer to bufctl */

	struct kmem_cache *pcache;			/* pointer to kmem_cache */
	uint16_t pageno;						/* page NO. */

	uint16_t active_objs;				/* active objects number */
	uint16_t total_objs;					/* total objects number */
};
#endif /* 0 */

struct kmem_bufctl
{
	struct kmem_bufctl *next;
	void *obj;
};

struct kmem_cache
{
	struct kmem_cache *prev;
	struct kmem_cache *next;

	char name[KMEM_CACHE_NAME_LEN];		/* name of kmem_cache */

	uint32_t flags;								/* flags for create */
	size_t buf_siz;							/* buffer's size */
	short active_objs;							/* active objects number */
	short total_objs;							/* total objects number */

	struct page_struct slab_head;				/* slab double list header */

	void (*constructor)(void *, size_t);
	void (*destructor)(void *, size_t);

	struct semaphore sem;

#ifdef MEMORY_DEBUG
	uint16_t pagecnt;
	uint16_t err_page;
	uint16_t err_bufctl;
#endif /* MEMORY_DEBUG */
};


void slab_init();
struct kmem_cache *kmem_cache_create(char *name, size_t size, int align, uint32_t flags,
	void (*constructor)(void *, size_t), void (*destructor)(void *, size_t));
void kmem_cache_destroy(struct kmem_cache *cache);
void *kmem_cache_alloc(struct kmem_cache *cache, int flags);
void kmem_cache_free(struct kmem_cache *cache, void *buf);
void *kmalloc(size_t size, int flags);
void kfree(void *p);

#endif /* __D_SLAB_H__ */
