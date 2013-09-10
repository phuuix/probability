/*
 * putchar.c
 */

#include <stdio.h>
#include <string.h>
#include <tty.h>

/*
 * output a character
 */
int putchar(int c)
{
	int r = 0;
	unsigned char b[4];

	b[0] = 0;	/* to first tty */
	b[1] = c;

	r = tty_fw_write(&systty[0], b+1, 1);
	if(r > 0)
		return c;
	else
		return EOF;
}
