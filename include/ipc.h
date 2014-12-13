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
 *   IPC interfaces and data structure
 *
 * @History
 *   AUTHOR         DATE           NOTES
 */

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
#define IPC_FLAG_FREEMEM        0x02            /* memory need to be freed upon destroy */

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
