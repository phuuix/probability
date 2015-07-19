#ifndef _CLI_H_
#define _CLI_H_

/* shell session */
struct shell_session
{
	void *data;							/* session's private data */

	int (*output)();					/* output line function */
};

int cmd_process(char *cmd);

#endif
