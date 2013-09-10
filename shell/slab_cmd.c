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
/*   slab_cmd.c                                0.3.0                     */
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

#ifdef SHELL_COMMAND
#include <shell.h>

/*
 * dump a kmem_cache
 */
int show_kmem_cache(struct shell_session *ss, int argc, char **argv)
{
	int k;
	k = atoi(argv[2]);

	if(k>=0 && k<MAX_KMEM_CACHE_NUMBER)
	{
		int slab_num = 0;
		int totel_active_objs = 0;
		struct kmem_slab *pslab;
		struct kmem_cache *cp;
		
		cp = &cache_array[k];
		if(cp->buf_siz == 0)
		{
			ss->output("Uninitialized cache.\n");
			return SHELL_RET_WARN;
		}
		if(sem_take(cp->sem, TIME_WAIT_FOREVER) == RERROR)
		{
			ss->output("sem_take error %d\n", cp->sem);
			return SHELL_RET_WARN;
		}

		pslab = cp->slab_head.next;
		while(pslab != &(cp->slab_head))
		{
			slab_num ++;
			if(cp->type == KMEM_CACHE_TYPE_SMALL)
			{
				ss->output("slab%d: totel_objs(%d) active_objs(%d) pageno(%d)\n", 
					slab_num, pslab->totel_objs, pslab->active_objs, pslab->pageno);
			}
			totel_active_objs += pslab->active_objs;

			pslab = pslab->next;
		}

		ss->output("kmem_cache %s: type(%d) size(%d) sem(%d) slab_num(%d) active_objs(%d)\n",
			cp->name, cp->type, cp->buf_siz, cp->sem, slab_num, totel_active_objs);
		ss->output(" ---- alloc_num(%d) free_num(%d) err_page(%d) err_slab(%d) err_bufctl(%d)\n",
			cp->alloc_num, cp->free_num, cp->err_page, cp->err_slab, cp->err_bufctl);

		sem_give(cp->sem);
	}
	else
		ss->output("Invalid kmem_cache number.\n");

	return SHELL_RET_WARN;
}
#endif /* SHELL_COMMAND */
