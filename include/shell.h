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
/*   shell.h                                   0.3.0                     */
/*                                                                       */
/* COMPONENT                                                             */
/*   Shell                                                               */
/*                                                                       */
/* DESCRIPTION                                                           */
/*   shell header file                                                   */
/*                                                                       */
/* CHANGELOG                                                             */
/*   AUTHOR         DATE                    NOTES                        */
/*   Bearix         2006-8-20               Version 0.3.0                */
/*************************************************************************/ 


#ifndef __D_SHELL_H__
#define __D_SHELL_H__

#include <sys/types.h>

#define NODE_VAR_MARK_LEFT	'['
#define NODE_VAR_MARK_RIGHT	']'

#define SHELL_KEY_HELP	'?'
#define SHELL_KEY_TAB	'\t'
#define SHELL_KEY_UP	82
#define SHELL_KEY_DOWN	81

/* node's type */
#define NODE_TYPE_CONSTANT	0
#define NODE_TYPE_VARIABLE	1

/* max arguments number */
#define SHELL_ARGUMENTS_MAX	24

#define SHELL_HISTORY_NUM	8
#define SHELL_BUFFER_LENGTH	256
#define SHELL_USERNAME_LENGTH	32
#define SHELL_PASSWORD_LENGTH	32

/* shell command function return value */
#define SHELL_RET_OK	0
#define SHELL_RET_WARN	1
#define SHELL_RET_ERROR	2
#define SHELL_RET_EXIT	3

/* shell command type */
#define SHELL_COMMAND_NORMAL	0
#define SHELL_COMMAND_HELP		1
#define SHELL_COMMAND_COMPLETE	2
#define SHELL_COMMAND_UPHIS		3
#define SHELL_COMMAND_DOWNHIS	4

/* macro */
#define DECLARE_COMMAND(cmd) \
	extern int cmd(struct shell_session *ss, int argc, char **argv)

struct shcmd
{
	struct shcmd *next;
	struct shcmd *prev;
	char command[SHELL_BUFFER_LENGTH];
};

/* shell session */
struct shell_session
{
	char name[8];						/* session's name */
	void *data;							/* session's private data */

	int (*input)();						/* get line function */
	int (*output)();					/* output line function */
	int (*feedback)();					/* feedback line function */
	int (*login)();						/* login function */

	struct shell_dict *cur_dict;		/* current dictionary */

	struct shcmd hist_head;
	struct shcmd hist_tail;
	struct shcmd *hist_now;
	struct shcmd *hist_free;
	struct shcmd history[SHELL_HISTORY_NUM];					/* history buffer */
	char command[SHELL_BUFFER_LENGTH];							/* current command */

	char username[SHELL_USERNAME_LENGTH];						/* user name */
	char password[SHELL_PASSWORD_LENGTH];						/* password */
};

/* shell node */
struct shell_node
{
	char *name;							/* node's name */
	char *helpstr;						/* help string */
	uint16_t type;						/* node's type */

	struct shell_node *node_list;		/* sub-node list */
	struct shell_node *next;		

	int (*func)(struct shell_session *ss, int argc, char **argv);
};

/* shell dictionary */
struct shell_dict
{
	char *name;							/* dictonary's name */
	struct shell_dict *parent;			/* parent dictionary */
	struct shell_dict *dict_list;		/* sub dictionary list */
	struct shell_dict *next;			/* next brother dictionary */
	struct shell_node *node_list;		/* node list */
};

/* shell command describle struct */
struct shell_cmd_desc
{
	char *cmd_str;						/* command string */
	char *help_str;						/* help string */
	int (*func)(struct shell_session *ss, int argc, char **argv);
};

char *shell_strtok(char **str, char separator);
char *shell_strdup(char *str);

int shell_complete(struct shell_session *ss, int argc, char **argv);
int shell_help(struct shell_session *ss, int argc, char **argv);

void shell_print_dict(struct shell_dict *root, int space, int (*output)());
void shell_print_node(struct shell_node *node, char *str, int len, int (*output)());
void shell_print_command(struct shell_dict *dict,int (*output)());

struct shell_dict *shell_find_dict(struct shell_dict *parent, char *name);
struct shell_node *shell_find_node(struct shell_dict *root, int argc, char **argv);
struct shell_dict *shell_add_dict(struct shell_dict *root, char *dictstr);
int shell_add_command(struct shell_dict *dict, int (*func)(), char *cmdstr, char *helpstr);

void shell_session_start(struct shell_session *ss);
struct shell_session *shell_session_create(char *name, struct shell_dict *root, 
	int (*input)(), int (*output)(), int (*feedback)(), int (*login)());

int shell_help(struct shell_session *ss, int argc, char **argv);
int shell_complete(struct shell_session *ss, int argc, char **argv);
int shell_history(struct shell_session *ss, int type);
int shell_history_append(struct shell_session *ss, char *cmd);

#endif /* __D_SHELL_H__ */
