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
 *   shell command
 *
 * @History
 *   AUTHOR         DATE                 NOTES
 *   puhuix           
 */

#include <config.h>

#include <command.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
/*
#include <dooloo_test.h>
*/
#include <bsp.h>
#include <task.h>
#include <tty.h>

struct shell_session *sys_default_ss;

static int cd_test(struct shell_session *ss, int argc, char **argv);
static int exit_test(struct shell_session *ss, int argc, char **argv);
static int read_line(char *buf, int len);
static int feedback_line();

/* DECLARE_COMMAND(gdb_trap); */
/* DECLARE_COMMAND(gdb_ipaddr_set); */
/* DECLARE_COMMAND(gdb_ipaddr_show); */
/* DECLARE_COMMAND(netif_ip_set); */
/* DECLARE_COMMAND(netif_ip_show); */
DECLARE_COMMAND(show_ering);
DECLARE_COMMAND(show_msgq);
DECLARE_COMMAND(show_mutex);
DECLARE_COMMAND(show_semaphore);
DECLARE_COMMAND(show_task);
DECLARE_COMMAND(show_task_all);
DECLARE_COMMAND(show_task_ready);
DECLARE_COMMAND(halt_dooloo);
DECLARE_COMMAND(task_hook);
DECLARE_COMMAND(show_scache_general);
DECLARE_COMMAND(show_scache);
DECLARE_COMMAND(show_slab);
DECLARE_COMMAND(show_page);
DECLARE_COMMAND(show_version);
DECLARE_COMMAND(ttypc_debug);
DECLARE_COMMAND(testcase_schedule);
DECLARE_COMMAND(testcase_semmtx);
DECLARE_COMMAND(test_task_exit);
DECLARE_COMMAND(test_task_destroy_begin);
DECLARE_COMMAND(test_task_destroy_end);

const struct shell_cmd_desc cmd_dooloo[] = {
/*	{"gdb trap", "gdb\n""enter gdb stub", gdb_trap}, */
/*	{"gdb ipaddr set [ip]", "gdb\n""ip address\n""set\n""new ip", gdb_ipaddr_set}, */
/*	{"gdb ipaddr show", "gdb\n""ip address\n""show", gdb_ipaddr_show}, */
/*	{"netif ip set [ip] [mask]", "netif\n""ip address\n""set\n""new ip\n""new netmask", netif_ip_set}, */
/*	{"netif ip show", "netif\n""ip address\n""show", netif_ip_show}, */
	{"show task all", "show\n""task\n""all tasks", show_task_all},
	{"show task ready", "show\n""task\n""ready tasks", show_task_ready},
	{"show task [name]", "show\n""task\n""task name", show_task},
/*	{"show messageq [q]", "show\n""message queue\n""message queue id", show_msgq},
	{"show mutex [m]", "show\n""mutex\n""mutex id", show_mutex},
	{"show semaphore [s]", "show\n""semaphore\n""semaphore id", show_semaphore},*/
	{"show scache info", "show\n""static cache\n""information", show_scache_general},
	{"show scache [s]", "show\n""static cache\n""static cache id", show_scache},
/*	{"show slab", "show\n""slab info\n", show_slab},
	{"show page", "show\n""page info\n", show_page}, */
	{"show version", "show\n""OS version", show_version},
	{"show command", "show\n""all commands in current directory", show_command},
	{"show directory", "show\n""all directories in current dictinary", show_directory},
	{"testcase schedule [n]", "testcase\n""schedule test\n""test number", testcase_schedule},
	{"testcase semmtx [n]", "testcase\n""semaphore mutex test\n""test number", testcase_semmtx},
/*	{"test", "run some test programs", cd_test},*/
	{"halt", "halt os", halt_dooloo},
	{"task hook enable", "task\n""hook function\n""set default hook", task_hook},
	{"task hook disable", "task\n""hook function\n""disable task hook", task_hook},
	{"ttypc debug [flags]", "tty for pc\n""debug\n""set debug flags [0|1]", ttypc_debug},
	{NULL, NULL, NULL}
};
/*
struct shell_cmd_desc cmd_test[] = {
	{"test memory", "test\n""memory test", test_memory},
	{"test schedule", "test\n""schedule test", test_schedule},
	{"test texit", "test\n""task exit", test_task_exit},
	{"test tdestroy begin", "test\n""task destroy\n""begin", test_task_destroy_begin},
	{"test tdestroy end", "test\n""task destroy\n""end", test_task_destroy_end},
	{"test event", "test\n""event test", test_event},
	{"test semaphore pc", "test\n""semaphore test\n""producter and consumer problem", test_semaphore_pc},
	{"test mutex pi", "test\n""mutex test\n""priority inherit problen", test_mutex_inherit},
	{"test messageq", "test\n""message queue test", test_messageq},
	{"test page alloc [n]", "test\n""page\n""alloc pages\n""pages number", test_page_alloc},
	{"test page free [p] [n]", "test\n""page\n""free pages\n""page NO.\n""pages number", test_page_free},
	{"test page show", "test\n""page\n""show page map", test_page_show},
	{"test slab [size] [num]", "test\n""slab\n""slab size\n""loop number", test_slab},
	{"exit", "back to upper", exit_test},
	{NULL, NULL, NULL}
};
*/
/*
 * add a command table to certain directory 
 *   dict: directory
 *   cmd_table: command description struct
 *   return: 1 on success, 0 on error
 */
static int command_table_add(struct shell_dict *dict, struct shell_cmd_desc *cmd_table)
{
	int i;

	if(dict == NULL)
		return 0;

	i=0;
	while(cmd_table[i].cmd_str != NULL)
	{
		int ret;
		ret = shell_add_command(dict, cmd_table[i].func, cmd_table[i].cmd_str, cmd_table[i].help_str);

		if(ret == 0)
			return 0;
		i++;
	}

	return 1;
}

/*
 * command task
 */
static void command_task(void *p)
{
	struct shell_session *ss;
	struct shell_dict *root;

	root = shell_add_dict(NULL, "dooloo");
//	test = shell_add_dict(root, "test");

	kprintf("Welcome to Dooloo shell!\n");
	kprintf("Whenever need help, just type '?'.\n");

	if(!command_table_add(root, cmd_dooloo))
		kprintf("add dooloo command table failed\n");
/*
	if(!command_table_add(test, cmd_test))
		kprintf("add test command table failed\n");
*/
	ss = shell_session_create("dooloo", root, read_line, printf, feedback_line, NULL);
	if(ss == NULL)
	{
		kprintf("Can't create dooloo session\n");
		return;
	}

	sys_default_ss = ss;
	shell_session_start(ss);
	kprintf("command task stoped.\n");
}

/*
 * init command, start shell
 */
void command_init()
{
	task_t t_cmd;

	t_cmd = task_create("tcmd", command_task, NULL, NULL, TCOMMAND_STACK_SIZE, TCOMMAND_PRIORITY, 0, 0);
	assert(t_cmd != -1);
	task_resume_noschedule(t_cmd);
}

/*
 * show commands
 */
int show_command(struct shell_session *ss, int argc, char **argv)
{
	ss->output("All commands in current directory:\n");
	shell_print_command(ss->cur_dict, ss->output);
	return SHELL_RET_OK;
}

/*
 * show directorys
 */
int show_directory(struct shell_session *ss, int argc, char **argv)
{
	ss->output("All directories in current directory:\n");
	shell_print_dict(ss->cur_dict, 0, ss->output);
	return SHELL_RET_OK;
}

/*
 * cd test directory
 */
static int cd_test(struct shell_session *ss, int argc, char **argv)
{
	struct shell_dict *test;

	test = shell_find_dict(ss->cur_dict, "test");
	if(test)
	{
		ss->cur_dict = test;
	}
	else
		ss->output("Can't find test directory!\n");

	return SHELL_RET_OK;
}

/*
 * exit test directory
 */
static int exit_test(struct shell_session *ss, int argc, char **argv)
{
	ss->cur_dict = ss->cur_dict->parent;
	return SHELL_RET_OK;
}

/*
 * get a line
 *   buf: receive buffer
 *   len: buffer length
 *   return: line length on success; 0 on failed
 */
static int read_line(char *buf, int len)
{
	if(gets(buf) != NULL)
	{
		return strlen(buf);
	}

	return 0;
}

static int feedback_line(char *buf, int len)
{
	int i;
	struct tty_evmail mail;

//	kprintf("feedback line: (%s)", buf);

	for(i=0; i<len; i++)
	{
		mail.tty = 0;
		mail.len = 1;
		mail.data[0] = buf[i];
		tty_post_mail(&mail);
	}
	return 0;
}
