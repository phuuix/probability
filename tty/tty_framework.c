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
 *   tty framework
 *
 * @History
 *   AUTHOR         DATE                 NOTES
 *   puhuix           
 */

#include <config.h>
#include <tty.h>
#include <task.h>
#include <ipc.h>
#include <assert.h>
#include <stddef.h>
#include <string.h>
#include <bsp.h>

struct tty_struct systty[MAX_TTY_NUMBER];

static msgq_t tty_outq;
static mbox_t tty_evbox;
static uint8_t tty_mbox_buf[sizeof(struct tty_evmail)*TTY_INPUT_MBOX_NUM];
static uint8_t tty_msgq_buf[TTY_OUTPUT_BUFFER_SIZE];

static void tty_input_task(void *p)
{
	struct tty_evmail mail;
	int ret, len;
	struct tty_struct *tty;
	int ttyno;

	while(1)
	{
		ret = mbox_pend(&tty_evbox, (uint32_t *)&mail, -1);
		ttyno = mail.tty;
		if(ret == ROK && ttyno < MAX_TTY_NUMBER)
		{
			tty = &systty[ttyno];
			len = mail.len;
//			kprintf("<mail.len=%d mail.data[0]=%d>", mail.len, mail.data[0]);

			if(tty->driver->read)
				len = (*(tty->driver->read))(tty, mail.data, len);

			if(len > 0 && tty->ldisc->read)
				len = (*(tty->ldisc->read))(tty, mail.data, len);
		}
	}
}

static void tty_output_task(void *p)
{
	unsigned char outbuf[TTY_WTBUF_SIZE];
	int ret, msglen;
	struct tty_struct *tty;
	int ttyno;

	while(1)
	{
		msglen = TTY_WTBUF_SIZE;
		ret = msgq_pend(&tty_outq, (char *)outbuf, (size_t *)&msglen, -1);
		ttyno = *outbuf;
		if(ret == ROK && ttyno < MAX_TTY_NUMBER)
		{
			outbuf[msglen] = 0;
			/* the first byte in outbuf is the tty No. */
			tty = &systty[ttyno];

			if(tty->ldisc->write)
				msglen = (*(tty->ldisc->write))(tty, outbuf+1, msglen-1);

			if(ret >= 0 && tty->driver->write)
				msglen = (*(tty->driver->write))(tty, outbuf+1, msglen);
		}
		else
		{
			assert(ret == ROK);
		}
	}
}

/*
 * init tty subsystem 
 */
void tty_init()
{
	task_t t;

	/* init related data structure */
	msgq_initialize(&tty_outq, TTY_OUTPUT_BUFFER_SIZE, tty_msgq_buf);

	mbox_initialize(&tty_evbox, sizeof(struct tty_evmail)/sizeof(int), TTY_INPUT_MBOX_NUM, tty_mbox_buf);

	/* create tty_input task */
	t = task_create("tinput", tty_input_task, NULL, NULL, TTTY_INPUT_STACK_SIZE, TTTY_INPUT_PRIORITY, 0, 0);
	assert(t != RERROR);
	task_resume_noschedule(t);

	/* create tty_output task */
	t = task_create("toutput", tty_output_task, NULL, NULL, TTTY_OUTPUT_STACK_SIZE, TTTY_OUTPUT_PRIORIY, 0, 0);
	assert(t != RERROR);
	task_resume_noschedule(t);
}

/*
 * register a tty
 */
int tty_register(char *name, struct tty_ldisc *ldisc, struct tty_driver *driver)
{
	int i, pos=MAX_TTY_NUMBER;

	if(name == NULL || ldisc == NULL || driver == NULL)
		return 0;

	/* check the name's unique */
	for(i=0; i<MAX_TTY_NUMBER; i++)
	{
		if((strlen(name) == strlen(systty[i].name)) && (strcmp(name, systty[i].name) == 0))
			return 0;

		if(((systty[i].flags & TTY_FLAG_BUSY) == 0) && (pos == MAX_TTY_NUMBER))
			pos = i;
	}

	if(pos < MAX_TTY_NUMBER)
	{
		strncpy(systty[pos].name, name, TTY_NAME_SIZE-1);
		systty[pos].ldisc = ldisc;
		systty[pos].driver = driver;
		sem_initialize(&systty[pos].rdsem, 0);
		mtx_initialize(&systty[pos].rdmtx);
		systty[pos].flags = TTY_FLAG_BUSY;
		return 1;
	}

	return 0;
}

/* 
 * unregister a tty 
 */
int tty_unregister(char *name)
{
	int i;

	if(name == NULL)
		return 0;

	/* check the name's unique */
	for(i=0; i<MAX_TTY_NUMBER; i++)
	{
		if((strlen(name) == strlen(systty[i].name)) && (strcmp(name, systty[i].name) == 0))
		{
			sem_destroy(&systty[i].rdsem);
			mtx_destroy(&systty[i].rdmtx);
			systty[i].flags = 0;
			return 1;
		}
	}

	return 0;
}

int tty_post_mail(struct tty_evmail *mail)
{
	if(mail && (tty_evbox.flag & IPC_FLAG_VALID))
		mbox_post(&tty_evbox, (uint32_t *)mail);

	return 0;
}

struct tty_struct * tty_fw_open(char *name, int flags)
{
	struct tty_struct *tty = NULL;
	int ret = 0;
	int i;

	/* check the name's unique */
	for(i=0; i<MAX_TTY_NUMBER; i++)
	{
		if((strlen(name) == strlen(systty[i].name)) && (strcmp(name, systty[i].name) == 0))
		{
			tty = &systty[i];
			break;
		}
	}

	if(tty)
	{
		tty->flags = tty->flags | flags | TTY_FLAG_OPENED;
		tty->rdpos = tty->wtpos = 0;
		tty->linepos = 0;

		if(tty->ldisc->open)
			ret = (*(tty->ldisc->open))(tty);

		if(ret >= 0 && tty->driver->open)
			ret = (*(tty->driver->open))(tty);
	}

	return tty;
}

void tty_fw_close(struct tty_struct *tty)
{
	if((tty->flags & (TTY_FLAG_BUSY|TTY_FLAG_OPENED)) == (TTY_FLAG_BUSY|TTY_FLAG_OPENED))
	{
		if(tty->ldisc->close)
			(*(tty->ldisc->close))(tty);

		if(tty->driver->close)
			(*(tty->driver->close))(tty);

		tty->flags = TTY_FLAG_BUSY;
	}
}

ssize_t tty_fw_read(struct tty_struct *tty, unsigned char *buf, size_t nr)
{
	int ret, siz = 0;

	if((tty->flags & (TTY_FLAG_BUSY|TTY_FLAG_OPENED)) != (TTY_FLAG_BUSY|TTY_FLAG_OPENED))
		return -1;

	if(buf == NULL)
		return -1;

	/* at first, try to read characters in tty read buffer */
	ret = mtx_pend(&tty->rdmtx, -1);
	if(ret == ROK)
	{
		if(tty->wtpos - tty->rdpos > 0)
		{
			if(nr < tty->wtpos - tty->rdpos)
			{
				siz = nr;
				memcpy(buf, tty->rdbuf, siz);
				tty->rdpos += siz;
			}
			else
			{
				siz = tty->wtpos - tty->rdpos;
				memcpy(buf, tty->rdbuf, siz);
				tty->rdpos = tty->wtpos = 0;
			}
			mtx_post(&tty->rdmtx);
			return siz;
		}
		mtx_post(&tty->rdmtx);
	}
	else
		return -1;

	/* no available characters, pending on rdsem */
	ret = sem_pend(&tty->rdsem, -1);
	if(ret == ROK)
	{
		/* characters available, read them out */
		ret = mtx_pend(&tty->rdmtx, -1);
		if(ret == ROK)
		{
			if(nr < tty->wtpos - tty->rdpos)
			{
				siz = nr;
				memcpy(buf, tty->rdbuf, siz);
				tty->rdpos += siz;
			}
			else
			{
				siz = tty->wtpos - tty->rdpos;
				memcpy(buf, tty->rdbuf, siz);
				tty->rdpos = tty->wtpos = 0;
			}
			mtx_post(&tty->rdmtx);
			return siz;
		}
		else
			return -1;
	}
	else
		return -1;

	return siz;
}

ssize_t tty_fw_write(struct tty_struct *tty, unsigned char *buf, size_t nr)
{
	int ret;

	if((tty->flags & (TTY_FLAG_BUSY|TTY_FLAG_OPENED)) != (TTY_FLAG_BUSY|TTY_FLAG_OPENED))
		return -1;

	if(buf == NULL)
		return -1;

	/* assert one character is reserved for tty No. */
	buf --;
	*buf = tty - systty;

	ret = msgq_post(&tty_outq, (char *)buf, nr+1);
	if(ret == ROK)
		return nr;
	else
		return -1;
}

int tty_fw_ioctl(struct tty_struct *tty, unsigned int cmd, unsigned long arg)
{
	int ret = 0;

	if((tty->flags & (TTY_FLAG_BUSY|TTY_FLAG_OPENED)) != (TTY_FLAG_BUSY|TTY_FLAG_OPENED))
		return -1;

	if(tty)
	{
		if(tty->ldisc->ioctl)
			ret = (*(tty->ldisc->ioctl))(tty, cmd, arg);

		if(ret >= 0 && tty->driver->ioctl)
			ret = (*(tty->driver->ioctl))(tty, cmd, arg);
	}

	return ret;
}

