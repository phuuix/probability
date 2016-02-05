/* tools for crash dump */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/*************************************************
 * work around from host tool crashdump.exe start
 *************************************************/
 #define CRASHDUMP
 
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned int ipc_t;
typedef int task_t;
struct dtask
{
	int dummy;
};

/*************************************************
 * work around from host tool crashdump.exe end
 *************************************************/
 
#include "../include/journal.h"

const char *str_task_state[] = {
	"D",
	"S",
	"R",
	"B"
};

const char *str_wakeup_cause[] = {
	"ROK",
	"RERROR",
	"RTIMEOUT",
	"RAGAIN",
	"RSIGNAL"
};

const char *str_class1_event[] = {
	"TASKSWIT",
	"TIMESTAM",
	"MBOXPOST",
	"MBOXPEND",
	"SEMPPOST",
	"SEMPPEND",
	"MUTXPOST",
	"MUTXPEND",
	"MSGQPOST",
	"MSGQPEND"
};

void dump_class1_journal_header(journal_class1_t *journal_class1_event)
{
	if(journal_class1_event->jtype < JOURNAL_TYPE_CLASS1MAX)
		printf("usec %06d t_current %02d event %s ", journal_class1_event->usec, journal_class1_event->t_current, str_class1_event[journal_class1_event->jtype]);
	else
		printf("usec %06d t_current %02d event unknown%d ", journal_class1_event->usec, journal_class1_event->t_current, journal_class1_event->jtype);
}

void dump_class1_journal(journal_class1_t *journal_class1_history, uint32_t nElement)
{
	int i;
	
	// dump info
	for(i=0; i<JOURNAL_CLASS1_MAXRECORD; i++)
	{
		switch(journal_class1_history[i].jtype)
		{
			case JOURNAL_TYPE_TASKSWITCH:
			{
				struct journal_taskswitch *jevent = (struct journal_taskswitch *)&journal_class1_history[i];
				
				if(jevent->tto_wakeup_cause > 4)
					jevent->tto_wakeup_cause = 1;
				
				printf("record %03d: ", i);
				dump_class1_journal_header(&journal_class1_history[i]);
				
				printf("tto %02d interrupt %d from_state %s from_delaying %d wakeup_cause=%s\n",
						jevent->tto, jevent->is_interrupt, str_task_state[jevent->tfrom_state], jevent->tfrom_isdelaying, str_wakeup_cause[jevent->tto_wakeup_cause]);
			}
			break;
			
			case JOURNAL_TYPE_TIMESTAMP:
			{
				struct journal_time *jevent = (struct journal_time *)&journal_class1_history[i];
				
				printf("record %03d: ", i);
				dump_class1_journal_header(&journal_class1_history[i]);
				
				printf("second %d\n", jevent->tv_sec);
			}
			break;
			
			case JOURNAL_TYPE_MBOXPOST:
			{
				struct journal_mboxpost *jevent = (struct journal_mboxpost *)&journal_class1_history[i];
				
				printf("record %03d: ", i);
				dump_class1_journal_header(&journal_class1_history[i]);
				
				printf("tipc %02d interrupt %d is_wakeup %d t_wakeup %02d\n", jevent->t_ipc, jevent->is_interrupt, jevent->is_wakeup, jevent->t_wakeup);
			}
			break;
				
			case JOURNAL_TYPE_MBOXPEND:
			{
				struct journal_mboxpend *jevent = (struct journal_mboxpend *)&journal_class1_history[i];
				
				printf("record %03d: ", i);
				dump_class1_journal_header(&journal_class1_history[i]);
				
				printf("tipc %02d interrupt %d timeout %d \n", jevent->t_ipc, jevent->is_interrupt, jevent->timeout);
			}
			break;
			
			case JOURNAL_TYPE_SEMPOST:
			{
				struct journal_sempost *jevent = (struct journal_sempost *)&journal_class1_history[i];
				
				printf("record %03d: ", i);
				dump_class1_journal_header(&journal_class1_history[i]);
				
				printf("tipc %02d interrupt %d is_wakeup %d t_wakeup %02d semvalue %d\n", jevent->t_ipc, jevent->is_interrupt, 
						jevent->is_wakeup, jevent->t_wakeup, jevent->semvalue);
			}
			break;
			
			case JOURNAL_TYPE_SEMPEND:
			{
				struct journal_sempend *jevent = (struct journal_sempend *)&journal_class1_history[i];
				
				printf("record %03d: ", i);
				dump_class1_journal_header(&journal_class1_history[i]);
				
				printf("tipc %02d interrupt %d timeout %d semvalue %d\n", jevent->t_ipc, jevent->is_interrupt, jevent->timeout, jevent->semvalue);
			}
			break;
			
			default:
			{
				printf("record %03d: ", i);
				dump_class1_journal_header(&journal_class1_history[i]);
				
				printf("data 0x%x\n", journal_class1_history[i].data);
			}
			break;
		}
	}
}

/* read hex in form of etc. 0x01234567
 */
int read_hex(FILE *fp, uint32_t *hexarray, int nMaxU32)
{
	int nU32 = 0;
	char *lineptr, *token;
	char line[256];
	
	lineptr = fgets(line, 256, fp);
#if 0
	while(lineptr && nU32 < nMaxU32)
	{
		token = strtok(lineptr, "0x");
		while(token)
		{
			sscanf(token, "0x%x", &hexarray[nU32]);
			nU32 ++;
			
			token = strstr(NULL, "0x");
		}
	}
#else
	while(lineptr && nU32 < nMaxU32)
	{
		token = strstr(lineptr, "0x");
		while(token)
		{
			sscanf(token, "0x%x", &hexarray[nU32]);
			//printf("get 0x%08x from %s\n", hexarray[nU32], token);
			nU32 ++;
			
			token = strstr(token+1, "0x");
		}
		
		lineptr = fgets(line, 256, fp);
	}
#endif
	
	return nU32;
}

journal_class1_t journal_class1_history[JOURNAL_CLASS1_MAXRECORD];

int main(int argc, char *argv[])
{
	FILE *fp;
	uint32_t nU32;
	
	if(argc < 2)
	{
		printf("Usage: crashdump <logfile>\n");
		return 0;
	}
	
	fp = fopen(argv[1], "r");
	if(fp == NULL)
	{
		printf("Can't open logfile: %s\n", argv[1]);
		return 0;
	}
	
	nU32 = read_hex(fp, (uint32_t *)journal_class1_history, JOURNAL_CLASS1_MAXRECORD*2);
	
	dump_class1_journal(journal_class1_history, nU32/2);
	
	close(fp);
	
	return 0;
}
