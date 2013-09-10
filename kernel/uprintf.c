/*
 * uprintf.c
 * user level printf
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

#define UPRINTF_BUFSIZE    200
#define UPRINTF_BUFNUM     32						/* must be power of 2 */
static char uprintf_buffer[UPRINTF_BUFNUM][UPRINTF_BUFSIZE];
static uint8_t uprintf_bufstat[UPRINTF_BUFNUM];
static uint16_t uprintf_buf_rdpt, uprintf_buf_wtpt; /* read and write point */

static task_t t_print;

#define UPRINT_MAX_BLOCK 64
static uint8_t uprintf_bitmap[UPRINT_MAX_BLOCK];


#define UPRINTF_LOGTIME  0x01
#define UPRINTF_LOGLEVEL 0x02
#define UPRINTF_LOGBLOCK 0x04
uint32_t uprintf_logflags = UPRINTF_LOGTIME | UPRINTF_LOGLEVEL | UPRINTF_LOGBLOCK;

extern int kprint_outchar(printdev_t *dev, char c);
extern int printi(printdev_t *dev, int i, int b, int sg, int width, int pad, int letbase);
extern int prints(printdev_t *dev, const char *string, int width, int pad);

static void uprintf_task(void *p)
{
	int flags, i;
	char *bufptr;
	
	for(;;)
	{
		flags = bsp_fsave();
		if(uprintf_bufstat[uprintf_buf_rdpt] == UPRINTF_BUFSTAT_BUSY)
		{
			bufptr = uprintf_buffer[uprintf_buf_rdpt];
			uprintf_bufstat[uprintf_buf_rdpt] = UPRINTF_BUFSTAT_IDLE;
			uprintf_buf_rdpt = (uprintf_buf_rdpt+1)&(UPRINTF_BUFNUM-1);
		}
		else
		{
			bufptr = NULL;
			task_block(NULL);
		}
		bsp_frestore(flags);
		
		if (bufptr)
		{
			PMC_PEG_SYS32_COUNTER(PMC_U32_nUprint,1);
			
			for(i=0; i<UPRINTF_BUFSIZE && bufptr[i]; i++)
			{
				// TODO: replace kprint_outchar with a parameter
				kprint_outchar(NULL, bufptr[i]);
			}
		}
	}
}

void uprintf_init()
{
	/* create tprint task */
	t_print = task_create("tprint", uprintf_task, NULL, NULL, TPRINT_STACK_SIZE, TPRINT_PRIORITY, 0, 0);
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

int uprintf(uint8_t level, uint8_t block_id, char *fmt,...)
{
	va_list parms;
	int result, flags;
	char *prtbuf;
	printdev_t sprintdev;

	/* check if the print can go through */
	if(!uprintf_enabled(level, block_id))
	{
		return 0;
	}
		
	/* get buffer ptr for print */
	flags = bsp_fsave();
	prtbuf = uprintf_buffer[uprintf_buf_wtpt];
	if(uprintf_bufstat[uprintf_buf_wtpt] == UPRINTF_BUFSTAT_BUSY)
	{
		uprintf_bufstat[uprintf_buf_wtpt] = UPRINTF_BUFSTAT_IDLE;
		
		PMC_PEG_SYS32_COUNTER(PMC_U32_nUprintOverwriten,1);
	}
	bsp_frestore(flags);
	
	sprintdev.internal = prtbuf;
	sprintdev.limit = UPRINTF_BUFSIZE-1;
	sprintdev.offset = 0;
	sprintdev.outchar = string_outchar;
	
	if(uprintf_logflags & UPRINTF_LOGTIME) // add 9 more bytes
	{
		printi(&sprintdev, tick(), 16, 0, 8 /* width */, 2 /* PAD_ZERO */, 'a');
		prints(&sprintdev, " ", 0, 0);
	}
	
	if(uprintf_logflags & UPRINTF_LOGLEVEL) // add 5 more bytes
	{
		prints(&sprintdev, "LVL", 0, 0);
		printi(&sprintdev, level, 16, 0, 0, 0, 'a');
		prints(&sprintdev, " ", 0, 0);
	}
	
	if(uprintf_logflags & UPRINTF_LOGBLOCK) // add 6 more bytes
	{
		prints(&sprintdev, "BLK", 0, 0);
		printi(&sprintdev, block_id, 16, 0, 2 /* width */, 2 /* PAD_ZERO */, 'a');
		prints(&sprintdev, " ", 0, 0);
	}
	
	va_start(parms,fmt);
	result = doiprintf(&sprintdev,fmt,parms);
	va_end(parms);
	
	/* terminate the string */
	if(result > sprintdev.limit)
		result = sprintdev.limit;
	prtbuf[result] = '\0';

	/* notify the t_print task */
	flags = bsp_fsave();
	uprintf_bufstat[uprintf_buf_wtpt] = UPRINTF_BUFSTAT_BUSY;
	uprintf_buf_wtpt = (uprintf_buf_wtpt+1) & (UPRINTF_BUFNUM-1);
	if(t_print) task_wakeup(t_print);
	bsp_frestore(flags);
	
	return(result);
}

