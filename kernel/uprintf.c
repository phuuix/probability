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
 *   user level printf
 *
 * @History
 *   AUTHOR         DATE                 NOTES
 *   puhuix           
 */

#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include <bsp.h>

#include "config.h"
#include "clock.h"
#include "s_alloc.h"
#include "ipc.h"
#include "siprintf.h"
#include "uprintf.h"
#include "journal.h"
#include "probability.h"

#define UPRINTF_BUFSTAT_IDLE 0
#define UPRINTF_BUFSTAT_BUSY 1


static char uprintf_buffer[UPRINTF_BUFNUM][UPRINTF_BUFSIZE];
static uint8_t uprintf_bufstat[UPRINTF_BUFNUM];
static uint16_t uprintf_buf_rdpt, uprintf_buf_wtpt; /* read and write point */

static task_t t_print;
static uint8_t uprintf_bitmap[UPRINT_MAX_BLOCK];


#define UPRINTF_LOGTIME  0x01
#define UPRINTF_LOGLEVEL 0x02
#define UPRINTF_LOGBLOCK 0x04
static uint32_t uprintf_logflags = UPRINTF_LOGTIME | UPRINTF_LOGLEVEL | UPRINTF_LOGBLOCK;

extern int kprint_outchar(printdev_t *dev, char c);
extern int printi(printdev_t *dev, int i, int b, int sg, int width, int pad, int letbase);
extern int prints(printdev_t *dev, const char *string, int width, int pad);
int vuprintf(uint8_t level, uint8_t block_id, char *fmt, va_list parms);

const char *uprintf_level[] = {
	"E", // emergency
	"A", // alert
	"C", // cirtical
	"E", // error
	"W", // warning
	"N", // notice
	"I", // informational
	"D", // debug
};

static void uprintf_task(void *p)
{
	int flags;
	uint16_t i, j;
	char *bufptr;
	uint16_t uprintf_buf_rdpt_local;

	/* set CLI block default enabled */
	uprintf_set_enable(UPRINT_INFO, UPRINT_BLK_CLI, 1);
	uprintf_set_enable(UPRINT_INFO, UPRINT_BLK_DEF, 1);

	/* set level<=warning default enabled */
	for(j=0; j<UPRINT_MAX_BLOCK; j++)
	{
		for(i=UPRINT_EMERG; i<UPRINT_INFO; i++)
			uprintf_set_enable(i, j, 1);
	}
	
	for(;;)
	{
		flags = bsp_fsave();
		uprintf_buf_rdpt_local = uprintf_buf_rdpt;
		bsp_frestore(flags);

		if(uprintf_bufstat[uprintf_buf_rdpt_local] == UPRINTF_BUFSTAT_IDLE)
		{
			bufptr = NULL;
			task_block(NULL);
		}
		else
			bufptr = uprintf_buffer[uprintf_buf_rdpt_local];
		
		if (bufptr)
		{
#ifdef INCLUDE_PMCOUNTER
			PMC_PEG_COUNTER(PMC_sys32_counter[PMC_U32_nUprint],1);
#endif
			
			for(i=0; i<UPRINTF_BUFSIZE && bufptr[i]; i++)
			{
				// TODO: replace kprint_outchar with a parameter
				kprint_outchar(NULL, bufptr[i]);
			}

			
			flags = bsp_fsave();
			if(uprintf_buf_rdpt_local == uprintf_buf_rdpt)
			{
				// uprintf_buf_rdpt isn't changed by vuprintf()
				uprintf_bufstat[uprintf_buf_rdpt_local] = UPRINTF_BUFSTAT_IDLE;
				uprintf_buf_rdpt = (uprintf_buf_rdpt+1)&(UPRINTF_BUFNUM-1);
			}
			bsp_frestore(flags);
		}
	}
}

void uprintf_init()
{
	/* create tprint task */
	t_print = task_create("tprint", uprintf_task, NULL, NULL, TPRINT_STACK_SIZE, TPRINT_PRIORITY, 20, 0);
	assert(t_print > 0);
	task_resume_noschedule(t_print);
}

int uprintf_enabled(uint8_t level, uint8_t block_id)
{
	if(level < UPRINT_MLEVEL && block_id < UPRINT_MAX_BLOCK)
	{
		return (uprintf_bitmap[block_id] & (1<<level));
	}
	
	return 0;
}

void uprintf_set_enable(uint8_t level, uint8_t block_id, uint8_t enable)
{
	if(level < UPRINT_MLEVEL && block_id < UPRINT_MAX_BLOCK)
	{
		if(enable)
			uprintf_bitmap[block_id] |= (1<<level);
		else
			uprintf_bitmap[block_id] &= ~(1<<level);
	}
}

uint32_t uprintf_get_flags()
{
	return uprintf_logflags;
}

uint32_t uprintf_set_flags(uint32_t flags)
{
	uint32_t old_flags;

	old_flags = uprintf_logflags;
	uprintf_logflags = flags;

	return old_flags;
}

int uprintf(uint8_t level, uint8_t block_id, char *fmt,...)
{
	va_list parms;
	int n;

	va_start(parms,fmt);
	n = vuprintf(level, block_id, fmt, parms);
	va_end(parms);
	
	return(n);
}

int vuprintf(uint8_t level, uint8_t block_id, char *fmt, va_list parms)
{
	int flags;
	char *prtbuf;
	printdev_t sprintdev;
	uint16_t uprintf_buf_wtpt_local;
    uint32_t tv_sec, tv_nsec;

	/* check if the print can go through */
	if(!uprintf_enabled(level, block_id))
	{
		return 0;
	}
		
	/* get buffer ptr for print and update write buffer ptr */
	flags = bsp_fsave();
	uprintf_buf_wtpt_local = uprintf_buf_wtpt;
	uprintf_buf_wtpt = (uprintf_buf_wtpt+1) & (UPRINTF_BUFNUM-1);  
	prtbuf = uprintf_buffer[uprintf_buf_wtpt_local];
	if(uprintf_bufstat[uprintf_buf_wtpt_local] == UPRINTF_BUFSTAT_BUSY)
	{
		// overwritten happens, must advance read pointer
		uprintf_bufstat[uprintf_buf_wtpt_local] = UPRINTF_BUFSTAT_IDLE;
		uprintf_buf_rdpt = (uprintf_buf_rdpt+1)&(UPRINTF_BUFNUM-1);
#ifdef INCLUDE_PMCOUNTER
		PMC_PEG_COUNTER(PMC_sys32_counter[PMC_U32_nUprintOverwriten],1);
#endif
	}
	bsp_frestore(flags);
	
	sprintdev.internal = prtbuf;
	sprintdev.limit = UPRINTF_BUFSIZE-1;
	sprintdev.offset = 0;
	sprintdev.outchar = string_outchar;
	
	if(uprintf_logflags & UPRINTF_LOGTIME) // add 9 more bytes
	{
        bsp_gettime(&tv_sec, &tv_nsec);
		printi(&sprintdev, tv_sec, 16, 0, 6 /* width */, 2 /* PAD_ZERO */, 'a');
        prints(&sprintdev, ".", 0, 0);
        printi(&sprintdev, tv_nsec/1000, 16, 0, 6 /* width */, 2 /* PAD_ZERO */, 'a');
		prints(&sprintdev, " ", 0, 0);
	}
	
	if(uprintf_logflags & UPRINTF_LOGLEVEL) // add 5 more bytes
	{
		prints(&sprintdev, uprintf_level[level], 0, 0);
		prints(&sprintdev, " ", 0, 0);
	}
	
	if(uprintf_logflags & UPRINTF_LOGBLOCK) // add 6 more bytes
	{
		prints(&sprintdev, "BLK", 0, 0);
		printi(&sprintdev, block_id, 16, 0, 2 /* width */, 2 /* PAD_ZERO */, 'a');
		prints(&sprintdev, " ", 0, 0);
	}
	
	doiprintf(&sprintdev,fmt,parms);
	
	/* terminate the string */
	prtbuf[sprintdev.offset] = '\0';

	/* after print complete, we can mark buffer busy */
	uprintf_bufstat[uprintf_buf_wtpt_local] = UPRINTF_BUFSTAT_BUSY;
	
	/* notify the t_print task */
	if(t_print)
		task_wakeup(t_print);

	return(sprintdev.offset);
}

int uprintf_default(char *fmt,...)
{
	va_list parms;
	int n;

	va_start(parms,fmt);
	n = vuprintf(UPRINT_INFO, UPRINT_BLK_DEF, fmt, parms);
	va_end(parms);

	return n;
}

