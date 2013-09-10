/*
 * net_task.h
 */
#ifndef __D_NET_TASK_H__
#define __D_NET_TASK_H__

struct net_msg
{
	void (*func)(void *arg);
	void *arg;
};

void net_task_init();
int net_task_job_add(void (*func)(), void *arg);

#endif /* __D_NET_TASK_H__ */
