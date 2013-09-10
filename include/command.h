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
/*   command.h                                 0.3.0                     */
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


#ifndef __D_COMMAND_H__
#define __D_COMMAND_H__

#include <shell.h>

void command_init();

int show_directory(struct shell_session *ss, int argc, char **argv);
int show_command(struct shell_session *ss, int argc, char **argv);

#endif /* __D_COMMAND_H__ */
