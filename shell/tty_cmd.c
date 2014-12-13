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
