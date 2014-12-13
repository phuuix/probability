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
 *   AUTHOR         DATE           NOTES
 */


#ifndef __D_COMMAND_H__
#define __D_COMMAND_H__

#include <shell.h>

void command_init();

int show_directory(struct shell_session *ss, int argc, char **argv);
int show_command(struct shell_session *ss, int argc, char **argv);

#endif /* __D_COMMAND_H__ */
