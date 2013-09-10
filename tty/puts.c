/*
 * puts.c
 */

#include <stdio.h>
#include <string.h>
#include <tty.h>

/*
 * output a string
 */
int puts(const char *s)
{
	int r = 0;
	unsigned char b[256];

	b[0] = 0;	/* to first tty */
	strncpy((char *)b+1, s, 255);

	r = tty_fw_write(&systty[0], b+1, strlen(s));
	
	return r;
}
