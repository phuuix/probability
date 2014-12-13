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
 *   
 *
 * @History
 *   AUTHOR         DATE                 NOTES
 *   puhuix           
 */

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

