/*
 * getchar.c
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

