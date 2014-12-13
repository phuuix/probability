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
 *   idle task
 *
 * @History
 *   AUTHOR         DATE           NOTES
 */

#include <idletask.h>
#include <assert.h>

#include "probability.h"

task_t t_idletask;
//static char idletask_stack[TILDE_STACK_SIZE];

/*
 * idle task
 */
static void idle_task(void *p)
{
//	kprintf("Idle task started.\n");
	for(;;);
}


/*
 * create idle task
 */
void idletask_init()
{
	t_idletask = task_create("tidle", idle_task, NULL, NULL, TILDE_STACK_SIZE, MAX_PRIORITY_LEVEL-1, 0, TASK_FLAGS_DONT_SLICE);
	task_resume_noschedule(t_idletask);
}
