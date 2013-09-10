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
/*   task_cmd.c                                0.3.0                     */
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
#include <stdlib.h>
#include <bsp.h>
#include <string.h>

extern struct dtask systask[MAX_TASK_NUMBER];

/*
 * dump a task
 *   t: task
 */
void task_dump(struct shell_session *ss, task_t t)
{
	ss->output("%3d ", t);		/* task id */
	ss->output("%-8s", systask[t].name);
	if(systask[t].state == TASK_STATE_DEAD)
		ss->output("%s  ", "dead");
	else if(systask[t].state == TASK_STATE_SUSPEND)
		ss->output("%s ", "suspend");
	else if(systask[t].state == TASK_STATE_READY)
		ss->output("%s   ", "ready");
	else if(systask[t].state == TASK_STATE_BLOCK)
		ss->output("%s   ", "block");
	ss->output("%5d ", systask[t].flags);
	ss->output("%3d ", systask[t].priority);
	ss->output("%10d ", systask[t].pri_origin);
	ss->output("%10u ", systask[t].timeslice_all);
	ss->output("%10u\n", systask[t].timeslice_left);
}


/*
 * show a task
 */
int show_task(struct shell_session *ss, int argc, char **argv)
{
	int i;

	for(i=0; i<MAX_TASK_NUMBER; i++)
	{
		if(strcmp((const char *)systask[i].name, argv[2]) == 0)
		{
			ss->output(" id name     state  flags pri  pri_origin    ticks    ticks_left\n");
			task_dump(ss, i);
			return SHELL_RET_OK;
		}
	}
	
	ss->output("unknown task\n");
	return SHELL_RET_WARN;
}

/*
 * show all tasks
 */
int show_task_all(struct shell_session *ss, int argc, char **argv)
{
	int i;

	ss->output(" id name     state  flags pri  pri_origin    ticks    ticks_left\n");
	for(i=0; i<MAX_TASK_NUMBER; i++)
	{
		if(systask[i].state != TASK_STATE_DEAD)
			task_dump(ss, i);
	}

	return SHELL_RET_OK;
}

/*
 * Dump task queue
 *   q: task queue
 */
void task_queue_dump(struct shell_session *ss, struct dtask *q)
{
	//struct dtask *t;
	//uint32_t f;

	ss->output(" id name     state  flags pri  pri_origin    ticks    ticks_left\n");
	ss->output(" TBD...\n");

/*
	f = fsave();
    t = q->next;
    while(t != q)
    {
		task_dump(ss, t-systask);
        t = t->next;
    }
	frestore(f);
*/
}

/*
 * show all tasks
 */
int show_task_ready(struct shell_session *ss, int argc, char **argv)
{
	task_queue_dump(ss, (struct dtask *)&sysready_queue);
	return SHELL_RET_OK;
}

/*
 * halt dooloo
 */
int halt_dooloo(struct shell_session *ss, int argc, char **argv)
{
	ss->output(" System is about to halt...\n");
	bsp_task_switch(0, 100*8);
	ss->output(" Done?\n");
	return SHELL_RET_OK;
}


static void sched_hook(struct dtask *from, struct dtask *to)
{
	kprintf("task_schedule(): from(%s, 0x%x) to(%s, 0x%x)\n", from->name, from->sp, to->name, to->sp);
}

/*
 * set task hook
 */
int task_hook(struct shell_session *ss, int argc, char **argv)
{
	if(argc > 2 && *argv[2] == 'e')
		task_set_schedule_hook(sched_hook);

	if(argc > 2 && *argv[2] == 'd')
		task_set_schedule_hook(NULL);

	return SHELL_RET_OK;
}
