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
 *   idletask.c                                0.3.0
 *
 * COMPONENT
 *   Kernel
 *
 * DESCRIPTION
 *
 *
 * CHANGELOG
 *   AUTHOR         DATE                    NOTES
 *   Bearix         2006-8-20               Version 0.3.0
 *   Bearix         2006-9-03               modified according new task_create api
 *************************************************************************/ 


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
