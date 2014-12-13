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
 *   A shell like CISCO style 
 *
 * @History
 *   AUTHOR         DATE                 NOTES
 *   puhuix           
 */

/*
 * TODO:
 * 1. sort node by name
 *    2001/12/11 bear done
 * 2. auto-complete
 * 3. relative directory
 *    2001/12/11 bear done
 * 4. history
 * 5. | more
 */

#include <string.h>
#include <shell.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <assert.h>
#include <bsp.h>

#include <siprintf.h>

char *shell_strdup(char *str)
{
	char *rstr;

	rstr = malloc(strlen(str));
	if(rstr)
		strcpy(rstr, str);

	return rstr;
}


char *shell_strtok(char **str, char separator)
{
	char *rstr;

	if(*str == NULL)
		return NULL;

	while(**str == separator)
	{
		**str = '\0';
		(*str)++;
	}

	if(**str == '\0')
		return NULL;

	rstr = *str;

	while(**str != '\0')
	{
		if(**str == separator)
		{
			**str = '\0';
			(*str)++;
			break;
		}
		(*str)++;
	}
	return rstr;
}


/*
 * get node type by node's name
 *   name: node's name
 *   return: type. 0--constant 1--variable
 */
static uint16_t shell_node_type(char *name)
{
	if(name[0] == NODE_VAR_MARK_LEFT &&
		name[strlen(name)-1] == NODE_VAR_MARK_RIGHT)
		return NODE_TYPE_VARIABLE;
	return NODE_TYPE_CONSTANT;
}


/*
 * get command type
 *   cmd: cmd string
 *   return: type
 */
static uint16_t shell_command_type(char *cmd)
{
	int len, ret=SHELL_COMMAND_NORMAL;

	len =strlen(cmd)-1;
	switch(cmd[len])
	{
		case SHELL_KEY_HELP:
			cmd[len] = '\0';
			ret = SHELL_COMMAND_HELP;
			break;

		case SHELL_KEY_TAB:
			cmd[len] = '\0';
			ret = SHELL_COMMAND_COMPLETE;
			break;

		case SHELL_KEY_UP:
			cmd[len] = '\0';
			ret = SHELL_COMMAND_UPHIS;
			break;

		case SHELL_KEY_DOWN:
			cmd[len] = '\0';
			ret = SHELL_COMMAND_DOWNHIS;
			break;
		
		default:
			ret = SHELL_COMMAND_NORMAL;
	}

	return ret;
}


/*
 * parse command string to argc and argv
 *   cmd: command string
 *   argv: argv
 *   return: argc
 */
static int shell_command_parse(char *cmd, char **argv)
{
	char **pcmd;
	char *t;
	int argc = 0;

	pcmd = &cmd;
	t = shell_strtok(pcmd, ' ');
	while(t)
	{
		argv[argc] = t;
		argc++;
		t = shell_strtok(pcmd, ' ');
	}
	return argc;
}


/*
 * print directory tree
 *   root: the root directory
 *   space: space number, at first should be 0
 */
void shell_print_dict(struct shell_dict *root, int space, int (*output)())
{
	int i;
	struct shell_dict *dict;

	if(root)
	{
		for(i=0; i<space; i++)
			output(" ");
		output("%s\n", root->name);
		
		space ++;
		dict = root->dict_list;
		while(dict)
		{
			shell_print_dict(dict, space+2, output);
			dict = dict->next;
		}
	}
}


/*
 * find a directory by name
 * It is a complete compare.
 *   parent: parent directory
 *   name: the name of directory
 *   return: the directory or NULL
 */
struct shell_dict *shell_find_dict(struct shell_dict *parent, char *name)
{
	if(parent)
	{
		struct shell_dict *dict;

		dict = parent->dict_list;
		while(dict)
		{
			if(strcmp(dict->name, name) == 0)
				return dict;
			else
				dict = dict->next;
		}
	}
	return NULL;
}


/*
 * print shell node
 *   node: shell node
 *   str: command string
 *   len: current string length, at first must be 0
 */
void shell_print_node(struct shell_node *node, char *str, int len, int (*output)())
{
	struct shell_node *nd;

	if(node)
	{
		siprintf(str+len, 100, "%s ", node->name);
		
		len += strlen(node->name)+1;
		nd = node->node_list;

		if(nd == NULL)
			output("  %s\n", str);
		else
		while(nd)
		{
			shell_print_node(nd, str, len, output);
			nd = nd->next;
		}
	}
}


/*
 * print all command in certain directory
 *   dict: directory
 */
void shell_print_command(struct shell_dict *dict , int (*output)())
{
	struct shell_node *node;
	char str[SHELL_BUFFER_LENGTH];

	node =dict->node_list;
	while(node)
	{
		shell_print_node(node, str, 0,output);
		node = node->next;
	}
}


/*
 * find a node in a directory by command string
 * this node can be compared partly unless a ambiguity exist.
 *   root: directory
 *   argc: arguments count
 *   argv: arguments
 *   return: node or NULL
 */
struct shell_node *shell_find_node(struct shell_dict *root, int argc, char **argv)
{
	struct shell_node *node, *cand_node, *var_node;
	int i=0, end=0;

	assert(root);

	node = root->node_list;
	cand_node = NULL;
	var_node = NULL;
	while(!end)
	{
		while(node)
		{
			if(node->type == NODE_TYPE_VARIABLE)
			{
				/* a variable node is always been selected */
				var_node = node;
			}
			if(strcmp(argv[i], node->name) == 0)
			{
				/* exactly compare */
				cand_node = node;
				break;
			}
			if(strncmp(argv[i], node->name, strlen(argv[i])) == 0)
			{
				if(cand_node == NULL)
					cand_node = node;
				else
				{
					/* find error: ambiguous command */
					cand_node = NULL;
					break;
				}
			}
			node = node->next;
		}

		if(cand_node == NULL && var_node == NULL)
			end = -1;	/* find error: no candicate node */
		else
		{
			if(cand_node == NULL)
				cand_node = var_node;	/* var_node has lowest proirity */

			if(i == argc-1)
				end = 1;	/* found */
			else
			{
				node = cand_node->node_list;
				cand_node = NULL;
				i++;
			}
		}
	}

	if(end == 1)
		return cand_node;
	else
		return NULL;
}


/*
 * add directorys according dictstr
 * dictstr is divide by character '/'.
 * all new directory is added and all old directory is not added.
 *   root: root directory
 *   dictstr: directory string
 *   return: NULL on failed, last directory on successful
 */
struct shell_dict *shell_add_dict(struct shell_dict *root, char *dictstr)
{
	struct shell_dict *dict = NULL, *parent_dict;
	char *dictname, **pdictstr;

//	assert(dictstr);

	parent_dict = root;
	pdictstr = &dictstr;
	dictname = shell_strtok(pdictstr, '/');

	while(dictname)
	{
		dict = shell_find_dict(parent_dict, dictname);
		if(dict == NULL)
		{
			/* malloc a directory */
			dict = (struct shell_dict *)malloc(sizeof(struct shell_dict));
			assert(dict);

			dict->name = dictname;
			dict->parent = parent_dict;
			dict->node_list = NULL;
			dict->dict_list = NULL;

			/* insert dict to parent_dict's dict_list */
			if(parent_dict)
			{
				dict->next = parent_dict->dict_list;
				parent_dict->dict_list = dict;
			}
		}

		parent_dict = dict;
		dictname = shell_strtok(pdictstr, '/');
	}

	return dict;
}


/*
 * add a command to a dictonary
 * command string is divide by character ' ', help string is divide by character '\n'
 *   dict: dictonary
 *   func: the function that to be executed
 *   cmdstr: command string
 *   helpstr: help string
 *   return: 1 on successful, 0 on failed.
 */
int shell_add_command(struct shell_dict *dict, 
	int (*func)(), char *cmdstr, char *helpstr)
{
	struct shell_node *node, **nlist;
	char *nodestr, *nodehelp;
	char **ncmd, **nhelp;

//	assert(dict);
//	assert(cmdstr);
//	assert(helpstr);

	ncmd = &cmdstr;
	nhelp = &helpstr;
	nodestr = shell_strtok(ncmd, ' ');
	nodehelp = shell_strtok(nhelp, '\n');
	node = dict->node_list;
	nlist = &(dict->node_list);
	while(nodestr)
	{
		while(node)
		{
			/* find a node whose name is equal to nodestr */
			if(strcmp(node->name, nodestr) == 0)
				break;
			else
				node = node->next;
		}
		if(node == NULL)
		{
			/* no found */

			/* get a new node */
			node = (struct shell_node *)malloc(sizeof(struct shell_node));
			assert(node);
			node->name = nodestr;
			node->helpstr = nodehelp;
			node->type = shell_node_type(node->name);
			node->node_list = NULL;
			node->func = NULL;

			/* insert new node to nlist sorting by name */
			{
				struct shell_node *n1, *n2 = NULL;
				n1 = *nlist;
				while(n1)
				{
					if(strcmp(n1->name, node->name) < 0)
					{
						n2 = n1;
						n1 = n1->next;
					}
					else
						break;
				}
				if(n2)
				{	/* insert new node behind n2 */
					node->next = n2->next;
					n2->next = node;
				}else
				{	/* insert new node to head of list */
					node->next = *nlist;
					*nlist = node;
				}
			}
		}
		nodestr = shell_strtok(ncmd, ' ');
		nodehelp = shell_strtok(nhelp, '\n');

		if(nodestr)
		{
			nlist = &(node->node_list);
			node = node->node_list;
		}
	}
	if(node && node->func == NULL)
	{
		node->func = func;
		return 1;
	}
	return 0;
}


/*
 * create shell session
 *   name: shell name
 *   root: shell's current directory
 *   input: input line function
 *   output: output line function
 *   login: login function
 *   return shell_session or NULL
 */
struct shell_session *shell_session_create(char *name, struct shell_dict *root, 
	int (*input)(), int (*output)(), int (*feedback)(), int (*login)())
{
	struct shell_session *ss;
	int i;

	if(name == NULL || input == NULL ||	output == NULL || root == NULL)
		return NULL;

	ss = (struct shell_session *)malloc(sizeof(struct shell_session));
	if(ss == NULL)
		return NULL;

	strncpy(ss->name, name, 8);
	ss->cur_dict = root;
	ss->data = NULL;
	ss->input = input;
	ss->output = output;
	ss->feedback = feedback;
	ss->login = login;

	/* history double link list 
	 * head <--> node ... node <--> tail
	 */
	memset(ss->command, 0, SHELL_BUFFER_LENGTH);
	ss->hist_head.command[0] = 0;
	ss->hist_tail.command[0] = 0;

	ss->hist_head.prev = NULL;
	ss->hist_head.next = &(ss->hist_tail);
	ss->hist_tail.prev = &(ss->hist_head);
	ss->hist_tail.next = NULL;
	ss->hist_now = NULL;
	ss->hist_free = NULL;
	for(i=0; i<SHELL_HISTORY_NUM; i++)
	{
		ss->history[i].next = ss->hist_free;
		ss->hist_free = &(ss->history[i]);
	}

	return ss;
}

#define FLAG_TITLE		0x01
#define FLAG_FEEDBACK	0x02

/*
 * start shell session
 *   ss: shell session
 */
void shell_session_start(struct shell_session *ss)
{
	char buffer[SHELL_BUFFER_LENGTH];
	int count;
	int ret = SHELL_RET_OK;
	int argc;
	char *argv[SHELL_ARGUMENTS_MAX];
	int flags = FLAG_TITLE;
	
	//assert(ss);
	if(ss->login)
	{
		ss->output("username: ");
		count = ss->input(ss->username, SHELL_USERNAME_LENGTH);
		ss->username[count] = '\0';
		ss->output("password: ");
		count = ss->input(ss->password, SHELL_PASSWORD_LENGTH);
		ss->password[count] = '\0';
		if(!ss->login(ss->username, ss->password))
		{
			ss->output("login failed.\n");
			return;
		}
	}
	
	/* now cur_dict must be set */
	if(ss->cur_dict == NULL)
		return;

	while(ret != SHELL_RET_EXIT)
	{
		uint16_t type;
		struct shell_node *node;

		if(flags & FLAG_TITLE) ss->output("%s# ", ss->cur_dict->name);

		/* session's command field contains characters that need to feedback */
		if(flags & FLAG_FEEDBACK)
		{
			/* try to feedback last command */
			ss->feedback(ss->command, strlen(ss->command));
			flags &= (~FLAG_FEEDBACK);
		}

		ss->command[0] = '\0';		/* empty command buffer */
		count = ss->input(buffer, SHELL_BUFFER_LENGTH);
		if(count <= 0)
		{
			/* view this as 'ENTER' key */
			ss->hist_now = &(ss->hist_tail);
			flags = FLAG_TITLE;
			continue;
		}

		type = shell_command_type(buffer);
		strcpy(ss->command, buffer);		/* save command */

		argc = shell_command_parse(buffer, argv);
		switch(type)
		{
		case SHELL_COMMAND_HELP:
			ret = shell_help(ss, argc, argv);
			flags = (FLAG_TITLE | FLAG_FEEDBACK);
			break;
		case SHELL_COMMAND_COMPLETE:
			ret = shell_complete(ss, argc, argv);
			flags &= FLAG_TITLE;				/* this field should be set to 0 */
			break;
		case SHELL_COMMAND_UPHIS:
		case SHELL_COMMAND_DOWNHIS:
			ret = shell_history(ss, type);
			if(ret == SHELL_RET_OK)
				flags = (FLAG_FEEDBACK);
			break;
		case SHELL_COMMAND_NORMAL:
			if(argc)
			{
				node = shell_find_node(ss->cur_dict, argc, argv);
				if(node == NULL)
					ss->output("  Unknown command.\n");
				else if(node->func == NULL)
					ss->output("  Incomplete command.\n");
				else
					ret = node->func(ss, argc, argv);

				shell_history_append(ss, ss->command);
			}
			flags = FLAG_TITLE;
			break;
		}
	}
}


/*
 * print help information
 */
int shell_help(struct shell_session *ss, int argc, char **argv)
{
	struct shell_node *node;

//	assert(ss);
//	assert(ss->cur_dict);
	if(argc == 0)
	{
		node = ss->cur_dict->node_list;
		while(node)
		{
			ss->output("%-30s%s\n", node->name, node->helpstr);
			node = node->next;
		}
	}
	else
	{
		node = shell_find_node(ss->cur_dict, argc, argv);
		if(node == NULL)
		{
			ss->output("  Unavailable help information.\n");
			return SHELL_RET_WARN;
		}

		node = node->node_list;
		while(node)
		{
			ss->output("%-30s%s\n", node->name, node->helpstr);
			node = node->next;
		}
	}
	return SHELL_RET_OK;
}


/*
 * complete shell command
 */
int shell_complete(struct shell_session *ss, int argc, char **argv)
{
	ss->output("auto complete function hasn't been finished\n");
	return SHELL_RET_OK;
}


/*
 * history handler
 */
int shell_history(struct shell_session *ss, int type)
{
	struct shcmd *pcmd, *head, *tail;
	int len;

	if(ss->hist_now == NULL)
	{
		/* no avilable history */
		return -1;
	}

	head = &(ss->hist_head);
	tail = &(ss->hist_tail);

	if(type == SHELL_COMMAND_UPHIS)
	{
		/* up key */
		if(ss->hist_now != head)
		{
			ss->hist_now = ss->hist_now->prev;
			pcmd = ss->hist_now;
		}
		else
			return -1;
	}
	else
	{
		/* down key */
		if(ss->hist_now != tail)
		{
			ss->hist_now = ss->hist_now->next;
			pcmd = ss->hist_now;
		}
		else
			return -1;
	}

	/* 
	 * delete the current line from the screen if line exists.
	 * maybe it is better to finish this task through call back function
	 */
	if((len = strlen(ss->command)) > 0)
	{
		memset(ss->command, 0x08, strlen(ss->command));
		ss->output(ss->command);
	}

	/* save the pcmd into ss->command */
	strcpy(ss->command, pcmd->command);

	return 0;
}


/*
 * history append
 */
int shell_history_append(struct shell_session *ss, char *cmd)
{
	struct shcmd *pcmd, *head, *tail;

	if(cmd == NULL || *cmd == '\0')
		return -1;

	head = &(ss->hist_head);
	tail = &(ss->hist_tail);

	if(ss->hist_free)
	{
		/* fetch a command buffer and save command */
		pcmd = ss->hist_free;
		ss->hist_free = ss->hist_free->next;
		strcpy(pcmd->command, cmd);
		
		/* insert the node (pcmd) into the end of queue */
		pcmd->prev = tail->prev;
		pcmd->next = tail;
		tail->prev->next = pcmd;
		tail->prev = pcmd;
	}
	else
	{
		/* overwrite the head of queue */
		pcmd = head->next;
		strcpy(pcmd->command, cmd);

		/* remove the node (pcmd) from the head of queue */
		head->next = pcmd->next;
		pcmd->next->prev = head;

		/* insert the node (pcmd) into the end of queue */
		pcmd->prev = tail->prev;
		pcmd->next = tail;
		tail->prev->next = pcmd;
		tail->prev = pcmd;
	}

	/* hist_now usually points to hist_tail */
	ss->hist_now = tail;

	return 0;
}

