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
 *   default line discipline for os
 *
 * @History
 *   AUTHOR         DATE                 NOTES
 *   puhuix           
 */

#include <config.h>
#include <tty.h>
#include <task.h>
#include <ipc.h>
#include <string.h>

#define LDISC_KEY_TAB			'\t'	/* tab */
#define LDISC_KEY_HELP			'?'		/* ? */
#define LDISC_KEY_UPHISTORY		82		/* up arrow */
#define LDISC_KEY_DOWNHISTORY	81		/* down arrow */
#define LDISC_KEY_BACKSPACE		0x08	/* backspace */
#define LDISC_KEY_ENTER			'\r'	/* enter */

static int ldisc_open(struct tty_struct *tty);
static void ldisc_close(struct tty_struct *tty);
static ssize_t ldisc_read(struct tty_struct *tty, unsigned char *buf, size_t nr);
static ssize_t ldisc_write(struct tty_struct *tty, unsigned char *buf, size_t nr);
static int ldisc_ioctl(struct tty_struct *tty, unsigned int cmd, unsigned long arg);

struct tty_ldisc sys_tty_ldisc_default = {
	ldisc_open,
	ldisc_close,
	ldisc_read,
	ldisc_write,
	ldisc_ioctl
};

static int ldisc_open(struct tty_struct *tty)
{
	return 0;
}

static void ldisc_close(struct tty_struct *tty)
{
}

static ssize_t ldisc_read(struct tty_struct *tty, unsigned char *buf, size_t nr)
{
	int i, ret;
	unsigned char echobuf[8];
	
	for(i=0; i<nr; i++)
	{
		switch(buf[i])
		{
			case LDISC_KEY_TAB:
			case LDISC_KEY_HELP:
				/* echo ENTER char */
				echobuf[1] = '\n';
				tty_fw_write(tty, echobuf+1, 1);

			case LDISC_KEY_UPHISTORY:
			case LDISC_KEY_DOWNHISTORY:
				tty->linebuf[tty->linepos++] = buf[i];
				ret = mtx_pend(&tty->rdmtx, -1);
				if(ret == ROK)
				{
					/* send line */
					memcpy(tty->rdbuf, tty->linebuf, tty->linepos);
					tty->rdpos = 0;
					tty->wtpos = tty->linepos;
					tty->linepos = 0;
					mtx_post(&tty->rdmtx);

					/* wake up client */
					sem_post(&tty->rdsem);
				}
				break;

			case LDISC_KEY_BACKSPACE:
				if(tty->linepos > 0)
				{
					tty->linepos --;

					/* echo char */
					echobuf[1] = buf[i];
					echobuf[2] = ' ';
					echobuf[3] = buf[i];
					tty_fw_write(tty, echobuf+1, 3);
				}
				break;

			case LDISC_KEY_ENTER:
				/* echo ENTER char */
				echobuf[1] = '\n';
				tty_fw_write(tty, echobuf+1, 1);

				ret = mtx_pend(&tty->rdmtx, -1);
				if(ret == ROK)
				{
					/* send line */
					memcpy(tty->rdbuf, tty->linebuf, tty->linepos);
					tty->rdpos = 0;
					tty->wtpos = tty->linepos;
					tty->linepos = 0;
					mtx_post(&tty->rdmtx);

//					kprintf("<tty->wtpos=%d>", tty->wtpos);

					/* wake up client */
					sem_post(&tty->rdsem);
				}
				break;

			default:
				/* echo char */
				echobuf[1] = buf[i];
				tty_fw_write(tty, echobuf+1, 1);

				tty->linebuf[tty->linepos++] = buf[i];
				if(tty->linepos == TTY_RDBUF_SIZE)
				{
					/* line buffer is full, send line */
					ret = mtx_pend(&tty->rdmtx, -1);
					if(ret == ROK)
					{
						memcpy(tty->rdbuf, tty->linebuf, tty->linepos);
						tty->rdpos = 0;
						tty->wtpos = tty->linepos;
						tty->linepos = 0;
						mtx_post(&tty->rdmtx);

//						kprintf("<tty->wtpos=%d>", tty->wtpos);
						/* wake up client */
						sem_post(&tty->rdsem);
					}
				}
		}
	}

	/* we don't care the return value */
	return nr;
}

static ssize_t ldisc_write(struct tty_struct *tty, unsigned char *buf, size_t nr)
{
	return nr;
}

static int ldisc_ioctl(struct tty_struct *tty, unsigned int cmd, unsigned long arg)
{
	return 0;
}

