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
/*   version.c                                 0.3.0                     */
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


#include <version.h>

#include <shell.h>

int show_version(struct shell_session *ss, int argc, char **argv)
{
	ss->output("  Dooloo Embedded Operating System version %s\n", DOOLOO_VERSION);
	ss->output("  Copyright: Puhui Xiong -- bearix@hotmail.com\n");

	return SHELL_RET_OK;
}

