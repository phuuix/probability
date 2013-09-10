/* a simple command line */

#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "config.h"
#include "task.h"
#include "bsp.h"
#include "cli.h"
#include "memory.h"

/* all commands:
 * i: show tasks
 * d: display memory
 * m: modify memory
 * s: show info
 * t: toggle trace
 * h|?: help
 */

static struct shell_session session;

void task_dump(struct shell_session *ss, task_t t);


int cmd_help(struct shell_session *ss, int argc, char **argv)
{
	ss->output("h|?:               display help info\n");
	ss->output("i:                 show tasks\n");
	ss->output("d <addr> [size]:   display memory\n");
	ss->output("m <addr> <value>:  modify memroy\n");
	ss->output("t:                 set trace level\n");
	ss->output("s <m>:             show \n");

	return 0;
}

int cmd_show_tasks(struct shell_session *ss, int argc, char **argv)
{
	int i;

	ss->output(" id name     state  flags pri  pri_origin    ticks    ticks_left\n");
	for(i=0; i<MAX_TASK_NUMBER; i++)
	{
		if(systask[i].state != TASK_STATE_DEAD)
			task_dump(ss, i);
	}

	return 0;
}

/*
 * dump a task
 *   t: task
 */
void task_dump(struct shell_session *ss, task_t t)
{
	ss->output("%3d ", t);		/* task id */
	ss->output("%-8s", systask[t].name);
	if(systask[t].state == TASK_STATE_DEAD)
		ss->output("%s  ", "dead");
	else if(systask[t].state == TASK_STATE_SUSPEND)
		ss->output("%s ", "suspend");
	else if(systask[t].state == TASK_STATE_READY)
		ss->output("%s   ", "ready");
	else if(systask[t].state == TASK_STATE_BLOCK)
		ss->output("%s   ", "block");
	ss->output("%5d ", systask[t].flags);
	ss->output("%3d ", systask[t].priority);
	ss->output("%10d ", systask[t].pri_origin);
	ss->output("%10u ", systask[t].timeslice_all);
	ss->output("%10u\n", systask[t].timeslice_left);
}

int cmd_display_mem(struct shell_session *ss, int argc, char **argv)
{
	uint32_t value, *addr;
	uint32_t i;
	
	if(argc < 2)
		return 0;

	value = strtoll(argv[1], NULL, 16);
	addr = (uint32_t *)value;
	
	if(argc > 2)
		value = strtol(argv[2], NULL, 10);
	else
		value = 4;
		
	for(i=0; i<value; i+=4)
	{
		ss->output(" 0x%x -- 0x%x\n", (uint32_t)addr, *addr);
		addr ++;
	}
	
	return 0;
}

int cmd_modify_mem(struct shell_session *ss, int argc, char **argv)
{
	uint32_t *addr;
	uint32_t value;
	
	if(argc < 3)
		return 0;

	value = strtoll(argv[1], NULL, 16);
	addr = (uint32_t *)value;
	value = strtol(argv[2], NULL, 10);

	ss->output("Set the value of addr 0x%x to 0x%x\n", addr, value);
	
	addr[0] = value;
	
	return 0;
}

int cmd_show(struct shell_session *ss, int argc, char **argv)
{
	if(argc < 2)
		return 0;

	if(argv[1][0] == 'm')
		memory_dump();
	else
		ss->output(" unknow argument: %s\n", argv[1]);

	return 0;
}




extern uint32_t g_trace_flags;
int cmd_toggle_trace(struct shell_session *ss, int argc, char **argv)
{
	if(argc < 2)
		g_trace_flags = ~g_trace_flags;
	else
		g_trace_flags = strtol(argv[1], NULL, 16);
	return 0;
}

char *cmd_strtok(char **str, char separator)
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
 * parse command string to argc and argv
 *   cmd: command string
 *   argv: argv
 *   return: argc
 */
static int cmd_parse(char *cmd, char **argv, int max)
{
	char **pcmd;
	char *t;
	int argc = 0;

	pcmd = &cmd;
	t = cmd_strtok(pcmd, ' ');
	while(t && argc<max)
	{
		argv[argc] = t;
		argc++;
		t = cmd_strtok(pcmd, ' ');
	}
	return argc;
}

int cmd_process(char *cmd)
{
	int argc;
	char *argv[6];
	char *ch = cmd;

	if(session.output == NULL)
		session.output = (int (*)())kprintf;
	
	/* convert non-print to blank */
	while(*ch){
		if(!isprint(*ch))
			*ch = ' ';
		ch ++;
	}
	
	argc = cmd_parse(cmd, argv, 6);
	if(argc < 0) return -1;
	
	if(strlen(argv[0]) != 1)
	{
		kprintf("unknown command: %s\n", argv[0]);
		return 0;
	}
	
	ch = argv[0];
	switch(*ch)
	{
		case 'h':
		case '?':
		{
			cmd_help(&session, argc, argv);
			break;
		}
		
		case 'i':
		{
			cmd_show_tasks(&session, argc, argv);
			break;
		}
		
		case 'd':
		{
			cmd_display_mem(&session, argc, argv);
			break;
		}
	
		case 'm':
		{
			cmd_modify_mem(&session, argc, argv);
			break;
		}

		case 't':
		{
			cmd_toggle_trace(&session, argc, argv);
			break;
		}

		case 's':
		{
			cmd_show(&session, argc, argv);
			break;
		}
		
		default:
			kprintf("unknown command: %s\n", argv[0]);
			break;
	}
	
	return 0;
}
