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

extern uint32_t tick();

void sys_init(void)
{
	
}

/** Create a new semaphore
 * @param sem pointer to the semaphore to create
 * @param count initial count of the semaphore
 * @return ERR_OK if successful, another err_t otherwise 
 *
 * Creates a new semaphore returns it through the sem pointer provided as argument to the function, in addition 
 * the function returns ERR_MEM if a new semaphore could not be created and ERR_OK if the semaphore was created. 
 * The count argument specifies the initial state of the semaphore. 
 */
err_t sys_sem_new(sys_sem_t *sem, u8_t count)
{
	int ret;
	
	ret = sem_initialize(sem, count);
	if(ret == ROK)
		return ERR_OK;
	else
		return ERR_MEM;
}

/** Delete a semaphore
 * @param sem semaphore to delete 
 *
 * Frees a semaphore created by sys_sem_new. Since these two functions provide the entry and exit point for all 
 * semaphores used by lwIP, you have great flexibility in how these are allocated and deallocated (for example, from 
 * the heap, a memory pool, a semaphore pool, etc). 
 */
void sys_sem_free(sys_sem_t *sem)
{
	sem_destroy(sem);
}

/** Signals a semaphore
 * @param sem the semaphore to signal 
 */
void sys_sem_signal(sys_sem_t *sem)
{
	sem_post(sem);
}

/** Wait for a semaphore for the specified timeout
 * @param sem the semaphore to wait for
 * @param timeout timeout in milliseconds to wait (0 = wait forever)
 * @return time (in milliseconds) waited for the semaphore
 *         or SYS_ARCH_TIMEOUT on timeout 
 */
u32_t sys_arch_sem_wait(sys_sem_t *sem, u32_t timeout)
{
	u32_t ret;
	int t;
	uint32_t start_time;

	start_time = tick();
	
	if(timeout == 0)
		t = TIME_WAIT_FOREVER;
	else
		t = timeout;

	if(sem_pend(sem, t) == ROK)
		ret = tick() - start_time;
	else
		ret = SYS_ARCH_TIMEOUT;

	return ret;
}

/** Create a new mbox of specified size
 * @param mbox pointer to the mbox to create
 * @param size (miminum) number of messages in this mbox
 * @return ERR_OK if successful, another err_t otherwise */
err_t sys_mbox_new(sys_mbox_t *mbox, int size)
{
	int ret;
	
    if(size == 0)
        size = 16;  // default size is 16

	ret = mbox_initialize(mbox, 2, size, NULL);
	
	if(ret == ROK)
		return ERR_OK;
	else
		return ERR_MEM;
}

/** Delete an mbox
 * @param mbox mbox to delete */
void sys_mbox_free(sys_mbox_t *mbox)
{
	mbox_destroy(mbox);
}

/** Post a message to an mbox - may not fail
 * -> blocks if full, only used from tasks not from ISR
 * @param mbox mbox to posts the message
 * @param msg message to post (ATTENTION: can be NULL) */
void sys_mbox_post(sys_mbox_t *mbox, void *msg)
{
	uint32_t mail;
	
	mail = (uint32_t)msg;
	mbox_post(mbox, &mail);
}

/** Try to post a message to an mbox - may fail if full or ISR
 * @param mbox mbox to posts the message
 * @param msg message to post (ATTENTION: can be NULL) */
err_t sys_mbox_trypost(sys_mbox_t *mbox, void *msg)
{
	int ret;
	uint32_t mail;
	
	mail = (uint32_t)msg;
	ret = mbox_post(mbox, &mail);

	if(ret == ROK)
		return ERR_OK;
	else
		return ERR_MEM;
}

/** Wait for a new message to arrive in the mbox
 * @param mbox mbox to get a message from
 * @param msg pointer where the message is stored
 * @param timeout maximum time (in milliseconds) to wait for a message
 * @return time (in milliseconds) waited for a message, may be 0 if not waited
           or SYS_ARCH_TIMEOUT on timeout
 *         The returned time has to be accurate to prevent timer jitter! */
u32_t sys_arch_mbox_fetch(sys_mbox_t *mbox, void **msg, u32_t timeout)
{
	u32_t ret;
	int t;
	uint32_t mail;
	uint32_t start_time;

	start_time = tick();

	if(timeout == 0)
		t = TIME_WAIT_FOREVER;
	else
		t = timeout;

	if(mbox_pend(mbox, (uint32_t *)&mail, t) == ROK)
	{
		ret = tick() - start_time;
		if(msg)
		*(uint32_t *)msg = mail;
	}
	else
		ret = SYS_ARCH_TIMEOUT;

	return ret;
}

/** Wait for a new message to arrive in the mbox
 * @param mbox mbox to get a message from
 * @param msg pointer where the message is stored
 * @param timeout maximum time (in milliseconds) to wait for a message
 * @return 0 (milliseconds) if a message has been received
 *         or SYS_MBOX_EMPTY if the mailbox is empty */
u32_t sys_arch_mbox_tryfetch(sys_mbox_t *mbox, void **msg)
{
	u32_t ret = 0;
	uint32_t mail;

	if(mbox_pend(mbox, (uint32_t *)&mail, TIME_WAIT_NONE) == ROK)
	{
		if(msg)
		*(uint32_t *)msg = mail;
	}
	else
		ret = SYS_ARCH_TIMEOUT;

	return ret;
}

/** The only thread function:
 * Creates a new thread
 * @param name human-readable name for the thread (used for debugging purposes)
 * @param thread thread-function
 * @param arg parameter passed to 'thread'
 * @param stacksize stack size in bytes for the new thread (may be ignored by ports)
 * @param prio priority of the new thread (may be ignored by ports) */
sys_thread_t sys_thread_new(const char *name, lwip_thread_fn thread, void *arg, int stacksize, int prio)
{
	task_t t;
	
	if(stacksize <= 0)
		stacksize = SYS_LWIP_THREAD_STACK_SIZE;

	t = task_create((char *)name, thread, arg, NULL, stacksize, prio, 50, 0);
    assert(t>0);
	task_resume_noschedule(t);

	return t;
}

/** Check if a sempahore is valid/allocated: return 1 for valid, 0 for invalid */
int sys_sem_valid(sys_sem_t *sem)
{
	return (sem->flag & IPC_FLAG_VALID);
}

/** Set a semaphore invalid so that sys_sem_valid returns 0 */
void sys_sem_set_invalid(sys_sem_t *sem)
{
	sem->flag &= (~IPC_FLAG_VALID);
}


void sys_msleep(u32_t ms)
{
	task_delay(ms/10);
}

/** Check if an mbox is valid/allocated: return 1 for valid, 0 for invalid */
int sys_mbox_valid(sys_mbox_t *mbox)
{
	return (mbox->flag & IPC_FLAG_VALID);
}

/** Set an mbox invalid so that sys_mbox_valid returns 0 */
void sys_mbox_set_invalid(sys_mbox_t *mbox)
{
	mbox->flag &= (~IPC_FLAG_VALID);
}



