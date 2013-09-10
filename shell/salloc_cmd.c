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
/*   salloc_cmd.c                              0.3.0                     */
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

#include <shell.h>
#include <task.h>
#include <s_alloc.h>
#include <stdlib.h>

extern S_CACHE s_cache_array[MAX_S_CACHE_NUMBER];
extern S_CACHE *s_cache_list;

/*
 * display s_cache general
 */
int show_scache_general(struct shell_session *ss, int argc, char **argv)
{
	S_CACHE *sc;
	int i=0;

	sc = s_cache_list;
	task_lock();
	while(sc)
	{
		i++;
		sc = sc->next;
	}
	task_unlock();

	ss->output("static cache information:\n");
	ss->output("  totel(%d) free(%d)\n", MAX_S_CACHE_NUMBER, i);
	return SHELL_RET_OK;
}


/*
 * show a scache
 */
int show_scache(struct shell_session *ss, int argc, char **argv)
{
	s_cache_t s;

	s = atoi(argv[2]);
	if(s>=0 && s<MAX_S_CACHE_NUMBER)
	{
		int i = 0;
		S_OBJ *o;

		ss->output("static cache '%s' infomation:\n", s_cache_array[s].name);
		ss->output("  factor %d\n", s_cache_array[s].factor);
		ss->output("  ready objects %d\n", s_cache_array[s].objs);

		task_lock();
		o = s_cache_array[s].free_obj_list;
		while(o)
		{
			if(i && i%10 == 0)
				ss->output("\n");
			ss->output("%x ", o);
			i++;
			o = o->next;
		}
		task_unlock();
		ss->output("\n  free objects %d\n", i);

		return SHELL_RET_OK;
	}

	ss->output("  Invalid scache.\n");
	return SHELL_RET_WARN;
}

