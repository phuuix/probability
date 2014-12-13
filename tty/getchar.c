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

#include <stdio.h>
#include <string.h>
#include <tty.h>

/*
 * get a character
 */
int getchar(void)
{
	int r = 0;
	unsigned char b[4];

	r = tty_fw_read(&systty[0], b, 1);
	if(r > 0)
		return b[0];
	else
		return EOF;
}

