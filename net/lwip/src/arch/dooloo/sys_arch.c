/*
 * sys_arch.c
 * For dooloo x86, reference to sys_arch.txt.
 */

#include "lwip/debug.h"

#include <string.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
//#include <unistd.h>

#include "lwip/sys.h"
#include "lwip/opt.h"
#include "lwip/stats.h"

/* dooloo relate header files */
#include <ipc.h>
#include <bsp.h>

struct timeoutlist {
  struct sys_timeouts timeouts;
  task_t pid;
};

static struct timeoutlist timeoutlist[SYS_LWIP_THREAD_MAX];

void sys_init(void)
{
	int i;

	for(i=0; i<SYS_LWIP_THREAD_MAX; i++)
		timeoutlist[i].pid = -1;
}

sys_sem_t sys_sem_new(u8_t count)
{
	return sem_create(0, count);
}

void sys_sem_free(sys_sem_t sem)
{
	sem_destroy(sem);
}

void sys_sem_signal(sys_sem_t sem)
{
	sem_post(sem);
}

u32_t sys_arch_sem_wait(sys_sem_t sem, u32_t timeout)
{
	u32_t ret;
	int t;
//	time_t end_time = 0, start_time = 0;

	if(timeout == 0)
		t = TIME_WAIT_FOREVER;
	else
		t = timeout;

	if(sem_pend(sem, t) == RTIMEOUT)
		ret = SYS_ARCH_TIMEOUT;
	else
		ret = 1;

	return ret;
}

sys_mbox_t sys_mbox_new(void)
{
	return mbox_create(0, 1, SYS_MBOX_SIZE);
}

void sys_mbox_free(sys_mbox_t mbox)
{
	mbox_destroy(mbox);
}

void sys_mbox_post(sys_mbox_t mbox, void *msg)
{
	u_long mail;
	if(msg) mail = (u_long)msg;
	mbox_post(mbox, (u_long *)&mail);
}

u32_t sys_arch_mbox_fetch(sys_mbox_t mbox, void **msg, u32_t timeout)
{
	u32_t ret;
	int t;
	u_long mail;

	if(timeout == 0)
		t = TIME_WAIT_FOREVER;
	else
		t = timeout;

	if(mbox_pend(mbox, (u_long *)&mail, t) == RTIMEOUT)
		ret = SYS_ARCH_TIMEOUT;
	else
	{
		ret = 1;
		if(msg)
		*(u_long *)msg = mail;
	}

	return ret;
}


struct sys_timeouts *sys_arch_timeouts(void)
{
	task_t pid;
	int i;

	pid = TASK_T(current);
	for(i=0; i<SYS_LWIP_THREAD_MAX; i++)
	{
		if(timeoutlist[i].pid != -1 && timeoutlist[i].pid == pid)
			return &(timeoutlist[i].timeouts);
	}
#if 0
	return NULL;
#else
	/*
	 * There is only one LWIP thread, so we can return the first timeoutlist structure safely.
	 * This modification avoids to create a LWIP thread for ARP timer if it works:)
	 */
	return &(timeoutlist[0].timeouts);
#endif
}


sys_thread_t sys_thread_new(void (* thread)(void *arg), void *arg, int prio)
{
	task_t t;
	static int counter = 0;
	char tname[8];

	if(counter >= SYS_LWIP_THREAD_MAX)
	{
		LWIP_PLATFORM_DIAG(("lwip thread number exceed!\n"));
		return -1;
	}

	sprintf(tname, "%s%d", SYS_LWIP_THREAD_NAME, counter);

	t = task_create(tname, thread, arg, 
		SYS_LWIP_THREAD_STACK_SIZE, prio, 0, 0);
	task_start(t);

	timeoutlist[counter].pid = t;

	counter++;

	return t;
}
