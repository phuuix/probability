/*
 * gets.c
 */

#include <stdio.h>
#include <string.h>
#include <tty.h>

/*
 * get a string
 */
char *gets(char *s)
{
	int r = 0;

	r = tty_fw_read(&systty[0], (unsigned char *)s, 256);
	if(r > 0)
	{
		s[r] = 0;
		return s;
	}
	else
		return NULL;
}
