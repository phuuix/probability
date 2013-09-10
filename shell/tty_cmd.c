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
/*   tty_cmd.c                                 0.3.0                     */
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
#include <stdlib.h>

extern void tty_pc_set_debug_flags(unsigned int flags);

/*
 * set task hook
 */
int ttypc_debug(struct shell_session *ss, int argc, char **argv)
{
	int flags;

	flags = atoi(argv[2]);
#if 0
	tty_pc_set_debug_flags(flags);
#endif
	return SHELL_RET_OK;
}
