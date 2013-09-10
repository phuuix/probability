/*************************************************************************/
/* The Dooloo kernel                                                     */
/* Copyright (C) 2004-2006 Xiong Puhui (Bearix)                          */
/* All Rights Reserved.                                                  */
/*                                                                       */
/* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS  */
/* THE PROPERTY OF DOOLOO RTOS DEVELOPMENT TEAM                          */
/*                                                                       */
/*************************************************************************/

/*************************************************************************
 *
 * FILE                                       VERSION
 *   ipc.h                                     0.3.0
 *
 * COMPONENT
 *   Kernel
 *
 * DESCRIPTION
 *   IPC interfaces and data structure
 *
 * CHANGELOG
 *   AUTHOR         DATE                    NOTES
 *   Bearix         2006-8-20               Version 0.3.0
 *************************************************************************/ 

#ifndef __D_IPC_H__
#define __D_IPC_H__

#include <task.h>
#include <list.h>
#include <buffer.h>

#define IPC_TYPE_MBOX			0x01
#define IPC_TYPE_MSGQ			0x02
#define IPC_TYPE_SEMAPHORE		0x03
#define IPC_TYPE_MUTEX			0x04

#define IPC_FLAG_VALID			0x01			/* IPC is valid */
#define IPC_FLAG_TQUEUE_FIFO	0x02			/* IPC is based on FIFO task queue */
#define IPC_FLAG_TQUEUE_PRIO	0x04			/* IPC is based on priority task queue */

#define IPC_MAX_LEN 12

struct ipc_common
{
	uint16_t type;					/* type */
	uint16_t flag;					/* flag */
	blockq_t taskq;
};

struct mailbox
{
	uint16_t type;					/* type */
	uint16_t flag;					/* flag */
	blockq_t taskq;					/* sleep task queue */

	struct queue_buffer buf;		/* mail buffer */
};

struct messageq
{
	uint16_t type;					/* type */
	uint16_t flag;					/* flag */
	blockq_t taskq;					/* sleep task queue */

	struct mem_buffer buf;			/* message buffer */
};

struct semaphore
{
	uint16_t type;					/* type */
	uint16_t flag;					/* flag */
	blockq_t taskq;					/* sleep task queue */

	int value;						/* semaphore's value */
	int tasks;						/* sleep task number */
};

struct mutex
{
	uint16_t type;					/* type */
	uint16_t flag;					/* flag */
	blockq_t taskq;					/* sleep task queue */

	int value;						/* mutex's value */
	int holdtimes;					/* holdtimes */
	int tasks;						/* sleep task number */

	struct dtask *owner;			/* mutex's owner */
};

typedef struct mailbox mbox_t;
typedef struct messageq msgq_t;
typedef struct semaphore sem_t;
typedef struct mutex mtx_t;
typedef struct ipc_common ipc_t;



int mbox_initialize(mbox_t *mbox, uint16_t usize, uint16_t unum, void *data);
void mbox_destroy(mbox_t *mbox);
int mbox_post(mbox_t *mbox, uint32_t *msg);
int mbox_pend(mbox_t *mbox, uint32_t *msg, int timeout);

int msgq_initialize(msgq_t *q, uint16_t bufsize, void *buf);
void msgq_destroy(msgq_t *q);
int msgq_post(msgq_t *msgq, char *msg, size_t msglen);
int msgq_pend(msgq_t *msgq, char *msg, size_t *msglen, int timeout);

int sem_initialize(sem_t *sem, int value);
void sem_destroy(sem_t *sem);
int sem_pend(sem_t *sem, int timeout);
int sem_post(sem_t *sem);

int mtx_initialize(mtx_t *mtx);
void mtx_destroy(mtx_t *mtx);
int mtx_pend(mtx_t *mtx, time_t timeout);
int mtx_post(mtx_t *mtx);

void ipc_init();


#endif /* __D_IPC_H__ */
