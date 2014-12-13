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

#include <shell.h>
#include <task.h>
#include <psignal.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <ipc.h>
#include <clock.h>
#include <bsp.h>
#include <string.h>

/*************************************************************************
 * test entry point
 *************************************************************************/
static void test_multitasks_start();
static void test_suspend_start();
static void test_taskpriority_start();
static void test_tasklock_start();
static void test_setopt_start();

static void test_pinherit_start();
static void test_multitake_start();
static void test_destroyipc_start();
static void test_readwrite_start(int n);
static void test_prodcons_start(int n);
static void test_ipctimeout0_start();

int testcase_schedule(struct shell_session *ss, int argc, char **argv)
{
	int n;

	n = atoi(argv[2]);
	switch(n)
	{
		case 1:
			test_multitasks_start();
			break;
		case 2:
			test_suspend_start();
			break;
		case 3:
			test_taskpriority_start();
			break;
		case 4:
			test_tasklock_start();
			break;
		case 5:
			test_setopt_start();
			break;
		default:
			printf("wrong test case number, correct test case number is 1~5\n");
	}
	return SHELL_RET_OK;
}

int testcase_semmtx(struct shell_session *ss, int argc, char **argv)
{
	int n;

	n = atoi(argv[2]);
	switch(n)
	{
		case 1:
			test_pinherit_start();
			break;
		case 2:
			test_multitake_start();
			break;
		case 3:
			test_destroyipc_start();
			break;
		case 4:
			n = atoi(argv[3]);
			test_readwrite_start(n);
			break;
		case 5:
			n = atoi(argv[3]);
			test_prodcons_start(n);
			break;
		case 6:
			test_ipctimeout0_start();
			break;
		default:
			printf("wrong test case number, correct test case number is 1~6\n");
	}
	return SHELL_RET_OK;
}
/*************************************************************************
 *  test cases related to schedule
 *************************************************************************/

/*****************************1: multiple tasks*****************************/
static void task_multitasks_taskA()
{
	int n = 0;
	while(1){printf("This is taskA: %d\n", n++); task_delay(50);}
}

static void task_multitasks_taskB()
{
	int n = 0;
	while(1){printf("This is taskB: %d\n", n++); task_delay(50);}
}

static void sched_hook(struct dtask *from, struct dtask *to)
{
	kprintf("task_schedule(): from(%s, 0x%x) to(%s, 0x%x)\n", from->name, from->sp, to->name, to->sp);
}


static void test_multitasks_start()
{
	task_t taskA, taskB;

    kprintf("create taskA with pri=200\n");
	taskA = task_create("ttaskA", task_multitasks_taskA, NULL, NULL, 0x1000, 200, 0, 0);
	task_resume_noschedule(taskA);

    kprintf("create taskB with pri=210\n");
	taskB = task_create("ttaskB", task_multitasks_taskB, NULL, NULL, 0x1000, 210, 0, 0);
	task_resume_noschedule(taskB);

	task_set_schedule_hook(sched_hook);
	
	kprintf("cmd task Delay 500 ticks\n");
	task_delay(500);

	kprintf("destroy taskA\n");
	task_destroy(taskA);

	kprintf("destroy taskB\n");
	task_destroy(taskB);

	task_set_schedule_hook(NULL);

	kprintf("testcase multitasks end\n");
}

/**************************2: suspend and resume****************************/
static task_t task_suspendA;

static void task_suspend_taskA()
{
	while(1){printf("msgA\n"); task_delay(50);}
}

static void task_suspend_taskB()
{
	task_delay(50); 
	printf("msgB\n");
	task_suspend(task_suspendA);
	task_delay(150);
}

static void task_suspend_taskC()
{
	task_delay(100); 
	printf("msgC\n");
	task_resume(task_suspendA);
}

static void test_suspend_start()
{
	task_t taskB, taskC;

	task_suspendA = task_create("ttaskA", task_suspend_taskA, NULL, NULL, 0x1000, 150, 0, 0);
	task_resume_noschedule(task_suspendA);

	taskB = task_create("ttaskB", task_suspend_taskB, NULL, NULL, 0x1000, 100, 0, 0);
	task_resume_noschedule(taskB);

	taskC = task_create("ttaskC", task_suspend_taskC, NULL, NULL, 0x1000, 120, 0, 0);
	task_resume_noschedule(taskC);
	
	task_delay(500);
	task_destroy(task_suspendA);
	task_destroy(taskB);
	task_destroy(taskC);
}

/***********************3: priority schedule******************************/
static void task_priority_taskA()
{
	while(1){printf("msgA\n"); task_delay(50);}
}

static void task_priority_taskB()
{
	while(1){printf("msgB\n"); task_delay(50);}
}

static void task_priority_taskC()
{
	task_delay(150);
	printf("msgC\n");
	task_delay(100);
}

static void test_taskpriority_start()
{
	task_t taskA, taskB, taskC;

	taskA = task_create("ttaskA", task_priority_taskA, NULL, NULL, 0x1000, 150, 0, 0);
	task_resume_noschedule(taskA);

	taskB = task_create("ttaskB", task_priority_taskB, NULL, NULL, 0x1000, 90, 0, 0);
	task_resume_noschedule(taskB);

	taskC = task_create("ttaskC", task_priority_taskC, NULL, NULL, 0x1000, 80, 0, 0);
	task_resume_noschedule(taskC);
	
	task_delay(500);
	task_destroy(taskA);
	task_destroy(taskB);
	task_destroy(taskC);
}
/**************************4: task lock and unlock*********************/
static void task_lock_taskA()
{
	task_lock();
	kprintf("\nmsgA1\n");
	printf("msgA2\n");
	kprintf("msgA3\n");
	task_unlock();
}

static void task_lock_taskB()
{
	while(1){printf("msgB\n"); task_delay(50);}
}

static void test_tasklock_start()
{
	task_t taskA, taskB;

	taskA = task_create("ttaskA", task_lock_taskA, NULL, NULL, 0x1000, 210, 0, 0);
	task_resume_noschedule(taskA);

	taskB = task_create("ttaskB", task_lock_taskB, NULL, NULL, 0x1000, 90, 0, 0);
	task_resume_noschedule(taskB);
	
	task_delay(500);
	task_destroy(taskA);
	task_destroy(taskB);
}
/*************************5: task set option******************************/
static task_t taskSetoptB;
static void task_setopt_taskA()
{
	unsigned char priority = 180;

	task_delay(1000);
	printf("msgA\n");
	
	task_setopt(taskSetoptB, TASK_OPTION_PRIORITY, &priority, sizeof(unsigned char));
}

static void task_setopt_taskB()
{
	int j = 8;
	while(j--){printf("msgB\n"); task_delay(500);}
}

static void test_setopt_start()
{
	task_t taskA;

	taskA = task_create("ttaskA", task_setopt_taskA, NULL, NULL, 0x1000, 150, 0, 0);
	task_resume_noschedule(taskA);

	taskSetoptB = task_create("ttaskB", task_setopt_taskB, NULL, NULL, 0x1000, 90, 0, 0);
	task_resume_noschedule(taskSetoptB);
}

/*************************************************************************
 *  test cases related to semaphore, mutex
 *************************************************************************/
/*************************1: priority inherit*****************************/
static mtx_t mtx_pinherit;
static void task_pinherit_taskA()
{
	uint32_t begin, now;

	printf("taskA\n");
	mtx_pend(&mtx_pinherit, -1);
	time(&begin);
	do{
		time(&now);
	}
	while(now - begin <= 2);
	mtx_post(&mtx_pinherit);
	printf("taskA\n");
}

static void task_pinherit_taskB()
{
	task_delay(10);
	mtx_pend(&mtx_pinherit, -1);
	printf("taskB\n");
	mtx_post(&mtx_pinherit);
}

static void task_pinherit_taskC()
{
	while(1)
	{
		printf("taskC\n");
		task_delay(50);
	}
}

static void test_pinherit_start()
{
	task_t taskA, taskB, taskC;

	mtx_initialize(&mtx_pinherit);
	
	taskA = task_create("ttaskA", task_pinherit_taskA, NULL, NULL, 0x1000, 250, 0, 0);
	task_resume_noschedule(taskA);
	taskB = task_create("ttaskB", task_pinherit_taskB, NULL, NULL, 0x1000, 210, 0, 0);
	task_resume_noschedule(taskB);
	taskC = task_create("ttaskC", task_pinherit_taskC, NULL, NULL, 0x1000, 220, 0, 0);
	task_resume_noschedule(taskC);

	task_delay(500);
	task_destroy(taskA);
	task_destroy(taskB);
	task_destroy(taskC);
	mtx_destroy(&mtx_pinherit);
}
/*************************2: recursive take*****************************/
static mtx_t mtx_multitake;
static void task_multitake_taskA()
{
	int ret;

	ret = mtx_pend(&mtx_multitake, -1);
	if(ret == 0) printf("pendA\n");
	mtx_pend(&mtx_multitake, -1);
	if(ret == 0) printf("pendA\n");
	mtx_pend(&mtx_multitake, -1);
	if(ret == 0) printf("pendA\n");
	mtx_post(&mtx_multitake);
	if(ret == 0) printf("postA\n");
	mtx_post(&mtx_multitake);
	if(ret == 0) printf("postA\n");
	mtx_post(&mtx_multitake);
	if(ret == 0) printf("postA\n");
}

static void task_multitake_taskB()
{
	int ret;

	ret = mtx_pend(&mtx_multitake, -1);
	if(ret == 0) printf("pendB\n");
	ret = mtx_pend(&mtx_multitake, -1);
	if(ret == 0) printf("pendB\n");
	ret = mtx_pend(&mtx_multitake, -1);
	if(ret == 0) printf("pendB\n");
	ret = mtx_post(&mtx_multitake);
	if(ret == 0) printf("postB\n");
	ret = mtx_post(&mtx_multitake);
	if(ret == 0) printf("postB\n");
	ret = mtx_post(&mtx_multitake);
	if(ret == 0) printf("postB\n");
}

static void test_multitake_start()
{
	task_t taskA, taskB;

	mtx_initialize(&mtx_multitake);

	taskA = task_create("ttaskA", task_multitake_taskA, NULL, NULL, 0x1000, 150, 0, 0);
	task_resume_noschedule(taskA);
	taskB = task_create("ttaskB", task_multitake_taskB, NULL, NULL, 0x1000, 150, 0, 0);
	task_resume_noschedule(taskB);

	task_delay(500);
	task_destroy(taskA);
	task_destroy(taskB);
	mtx_destroy(&mtx_multitake);
}
/*************************3: destroy semaphare**************************/
static sem_t sem_destroyipc;
static void task_destroyipc_taskA()
{
	int ret;

	printf("taskA\n");
	ret = sem_pend(&sem_destroyipc, -1);
	printf("taskA: ret = %d\n", ret);
	while(1) task_delay(10);
}

static void task_destroyipc_taskB()
{
	int ret;

	printf("taskB\n");
	ret = sem_pend(&sem_destroyipc, -1);
	printf("taskB: ret = %d\n", ret);
	while(1) task_delay(10);
}

static void test_destroyipc_start()
{
	task_t taskA, taskB;

	sem_initialize(&sem_destroyipc, 1);

	taskA = task_create("ttaskA", task_destroyipc_taskA, NULL, NULL, 0x1000, 150, 0, 0);
	task_resume_noschedule(taskA);
	taskB = task_create("ttaskB", task_destroyipc_taskB, NULL, NULL, 0x1000, 150, 0, 0);
	task_resume_noschedule(taskB);

	task_delay(200);
	sem_destroy(&sem_destroyipc);

	task_delay(300);
	task_destroy(taskA);
	task_destroy(taskB);
}
/*************************4: reader/writer, using mutex**************************/
static sem_t sem_rw;
static mtx_t mtx_rw;
static int rw_task_finished;
static task_t taskrw[4];

static void task_reader(void *p)
{
	int n = *(int *)p;
	int r, readcount = 0;
	int m;

	if(n <= 0) n = 100;
	while(n--)
	{
		r = mtx_pend(&mtx_rw, TIME_WAIT_FOREVER);
		assert(r == ROK);
		readcount++;
		if( readcount==1)
		{
			r = sem_pend(&sem_rw, TIME_WAIT_FOREVER);
			assert(r == ROK);
		}
		mtx_post(&mtx_rw);
		
		/* read operation */
		m = n;
		while(m --);
		
		r = mtx_pend(&mtx_rw, TIME_WAIT_FOREVER);
		assert(r == ROK);
		readcount--;
		if(readcount==0) sem_post(&sem_rw);
		mtx_post(&mtx_rw);
	}
	rw_task_finished ++;
}

static void task_writer(void *p)
{
	int n = *(int *)p;
	int r;
	int m;

	if(n <= 0) n = 100;
	while(n--)
	{
		r = sem_pend(&sem_rw, TIME_WAIT_FOREVER);
		assert(r == ROK);

		/* writer operation */
		m = n;
		while(m --);

		sem_post(&sem_rw);
	}
	rw_task_finished ++;
}

static void test_readwrite_start(int n)
{
	rw_task_finished = 0;
	
	sem_initialize(&sem_rw, 1);
	
	mtx_initialize(&mtx_rw);

	taskrw[0] = task_create("tRA", task_reader, &n, NULL, 0x1000, 220, 10, 0);
	task_resume_noschedule(taskrw[0]);

	taskrw[1] = task_create("tRB", task_reader, &n, NULL, 0x1000, 220, 10, 0);
	task_resume_noschedule(taskrw[1]);

	taskrw[2] = task_create("tRC", task_reader, &n, NULL, 0x1000, 220, 10, 0);
	task_resume_noschedule(taskrw[2]);

	taskrw[3] = task_create("tWA", task_writer, &n, NULL, 0x1000, 220, 10, 0);
	task_resume_noschedule(taskrw[3]);

	/* wait test task to exit */
	printf("Reader/Writer started\n");
	while(rw_task_finished < 4) task_delay(50);
	printf("Reader/Writer finished\n");
}

/*************************5: productor/consumer, using semarphore***************/
#define PC_LISTNODE_NUMBER 10
struct pctest_list{
	struct pctest_list *next;
	char value;
};

static struct pctest_list pcarray[PC_LISTNODE_NUMBER];
static struct pctest_list *pclist1, *pclist2;
static int pc_finished_tasks;
static sem_t sem_empty, sem_full, mtx_pc;

/*
 * product task
 */
static void task_product(void *p)
{
	int n = *(int *)p;
	char v;
	int r;
	struct pctest_list *node;

	if(n <= 0) n = 100;
	v = current->name[2];
	while(n--)
	{
		r = sem_pend(&sem_empty, TIME_WAIT_FOREVER);
		assert(r == ROK);
		r = sem_pend(&mtx_pc, TIME_WAIT_FOREVER);
		assert(r == ROK);

		/* move a node from list1 to list2 */
		SINGLE_LIST_REMOVE(&pclist1, &node);
		assert(node != NULL);
		node->value = v;
		SINGLE_LIST_INSERT(&pclist2, node);

		sem_post(&mtx_pc);

		sem_post(&sem_full);
	}
	pc_finished_tasks++;
}

/*
 * consume task
 */
static void task_consume(void *p)
{
	int n = *(int *)p;
	char v;
	int r;
	struct pctest_list *node;

	if(n <= 0) n = 100;
	v = current->name[2];
	while(n--)
	{
		r = sem_pend(&sem_full, TIME_WAIT_FOREVER);
		assert(r == ROK);
		r = sem_pend(&mtx_pc, TIME_WAIT_FOREVER);
		assert(r == ROK);

		/* move a node from list2 to list1 */
		SINGLE_LIST_REMOVE(&pclist2, &node);
		assert(node != NULL);
		node->value = v;
		SINGLE_LIST_INSERT(&pclist1, node);

		sem_post(&mtx_pc);

		sem_post(&sem_empty);
	}
	pc_finished_tasks ++;
}


static void test_prodcons_start(int n)
{
	task_t tt;
	int i;
	struct pctest_list *node;

	pc_finished_tasks = 0;

	/* initialize list */
	pclist1 = NULL;
	pclist2 = NULL;
	for(i=PC_LISTNODE_NUMBER-1; i>=0; i--)
	{
		SINGLE_LIST_INSERT(&pclist1, &pcarray[i]);
	}

	/* create ipc */
	sem_initialize(&sem_empty, PC_LISTNODE_NUMBER);

	sem_initialize(&sem_full, 0);

	sem_initialize(&mtx_pc, 1);

	/* create tasks */
	tt = task_create("tPA", task_product, &n, NULL, 0x1000, 220, 1, 0);
	assert(tt != RERROR);
	task_resume_noschedule(tt);

	tt = task_create("tPB", task_product, &n, NULL, 0x1000, 220, 1, 0);
	assert(tt != RERROR);
	task_resume_noschedule(tt);

	tt = task_create("tPC", task_product, &n, NULL, 0x1000, 220, 1, 0);
	assert(tt != RERROR);
	task_resume_noschedule(tt);

	tt = task_create("tCA", task_consume, &n, NULL, 0x1000, 220, 1, 0);
	assert(tt != RERROR);
	task_resume_noschedule(tt);

	tt = task_create("tCB", task_consume, &n, NULL, 0x1000, 220, 1, 0);
	assert(tt != RERROR);
	task_resume_noschedule(tt);

	tt = task_create("tCC", task_consume, &n, NULL, 0x1000, 220, 1, 0);
	assert(tt != RERROR);
	task_resume_noschedule(tt);

	printf("Producter/Consumer test started\n");

	/* wait test task to exit */
	while(pc_finished_tasks < 6) task_delay(50);
	
	printf("Producter/Consumer test finished\n");

	/* dump list */
	printf("elements in list1: ");
	node = pclist1;
	while(node)
	{
		printf("%d ", node->value);
		node = node ->next;
	}
	printf("\n");
	printf("elements in list2: ");
	node = pclist2;
	while(node)
	{
		printf("%d ", node->value);
		node = node ->next;
	}
	printf("\n");
}

/*************************6: timeout=0*********************************************/
static sem_t sem_timeout0;
static void task_ipctimeout0_taskA(void *p)
{
	printf("taskA\n");
	sem_pend(&sem_timeout0, TIME_WAIT_FOREVER);
	task_delay(200);
	sem_post(&sem_timeout0);
}

static void task_ipctimeout0_taskB(void *p)
{
	int n = 0;
	
	printf("taskB\n");
	while(sem_pend(&sem_timeout0, TIME_WAIT_NONE) == RAGAIN)
		n++;

	sem_post(&sem_timeout0);
	printf("pend times %d\n", n);
}

static void test_ipctimeout0_start()
{
	task_t tt;
	
	sem_initialize(&sem_timeout0, 1);
	
	tt = task_create("taskA", task_ipctimeout0_taskA, NULL, NULL, 0x1000, 220, 0, 0);
	assert(tt != RERROR);
	task_resume_noschedule(tt);

	tt = task_create("taskB", task_ipctimeout0_taskB, NULL, NULL, 0x1000, 220, 0, 0);
	assert(tt != RERROR);
	task_resume_noschedule(tt);

	task_delay(500);
}

/*************************************************************************
 *  test cases related to signal, mailbox and message queue
 *************************************************************************/
/*************************1: signal action****************************************/
#if 0
static int sig_recved[2];

static void sig_handler(int sig)
{
	if(sig == 1)
		sig_recved[0] = 1;
	if(sig == 7)
		sig_recved[1] = 1;
}

static void task_signal_taskA(void *p)
{
	struct sigaction action;

	sig_recved[0] = 0;

	action.sa_handler = sig_handler;
	action.sa_mask = 0;
	action.sa_flags = 0;

	sigaction(1, &action, NULL);

	printf("%s: waiting to receive signal...\n", current->name);

	while(sig_recved[0] == 0);

	printf("%s: received signal 1\n", current->name);
}

static void task_signal_taskB(void *p)
{
	struct sigaction action;

	sig_recved[1] = 0;

	action.sa_handler = sig_handler;
	action.sa_mask = 0;
	action.sa_flags = 0;

	sigaction(7, &action, NULL);

	printf("%s: waiting to receive signal...\n", current->name);

	while(sig_recved[1] == 0) task_delay(50);

	printf("%s: received signal 7\n", current->name);
}


static void test_signal_start()
{
	static task_t tsig1, tsig2;

	tsig1 = task_create("tsig1", task_signal_taskA, NULL, NULL, 0x1000, 220, 0, 0);
	task_resume_noschedule(tsig1);

	tsig2 = task_create("tsig2", task_signal_taskB, NULL, NULL, 0x1000, 210, 0, 0);
	task_resume_noschedule(tsig2);

	task_delay(470);

	sig_task(tsig1, 1);
	sig_task(tsig2, 7);
}
#endif

