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
 
#include "journal.h"

const char *str_task_state[] = {
	"DEAD",
	"SUSPEND",
	"READY",
	"BLOCK"
};

const char *str_ipc_type[] = {
	"MBOX",
	"MSGQ",
	"SEMP",
	"MUTX"
};

const char *str_wakeup_cause[] = {
	"ROK",
	"RERROR",
	"RTIMEOUT",
	"RAGAIN",
	"RSIGNAL"
};

void crashdump_class1_journal(FILE *fp)
{
	char *str;
	char buf[256];
	int index=0, i;
	journal_class1_t journal_class1_history[JOURNAL_CLASS1_MAXRECORD];
	uint32_t *ptr1, *ptr2;
	struct journal_taskswitch *j_taskswitch;
	struct journal_ipcop *j_ipcop;
	
	// find "Class1 journal"
	while(str = fgets(buf, 256, fp))
	{
		str = strstr(buf, "Class1 journal");
		if(str)
			break;
	}
	
	if(str == NULL)
	{
		printf("warning: can't find Class1 journal\n");
		return;
	}
	
	sscanf(str, "Class1 journal (index=%d):", &index);
	
	// read all records
	for(i=0; i<JOURNAL_CLASS1_MAXRECORD; i+=2)
	{
		str = fgets(buf, 256, fp);
		if(str == NULL) break;
		
		//printf("buf=%s\n", buf);
		ptr1 = (uint32_t *)&journal_class1_history[i];
		ptr2 = (uint32_t *)&journal_class1_history[i+1];
		sscanf(str, " 0x%08x 0x%08x 0x%08x    0x%08x 0x%08x 0x%08x\n", &ptr1[0], &ptr1[1], &ptr1[2], &ptr2[0], &ptr2[1], &ptr2[2]);
	}
	
	// dump info
	for(i=0; i<JOURNAL_CLASS1_MAXRECORD; i++)
	{
		unsigned int wakeup_cause = 0;
		
		//ptr1 = (uint32_t *)&journal_class1_history[i];
		//printf(" 0x%08x 0x%08x 0x%08x\n", ptr1[0], ptr1[1], ptr1[2]);
		
		switch(journal_class1_history[i].ipcop.jtype)
		{
			case JOURNAL_TYPE_TASKSWITCH:
				j_taskswitch = &(journal_class1_history[i].taskswitch);
				wakeup_cause = j_taskswitch->tto_wakeup_cause;
				if(wakeup_cause > 4)
					wakeup_cause = 1;
				printf("record %02d: TTI=0x%08x TASKSWITCH %02d-->%02d wakeup_cause=%s tfrom_state=%s tfrom_flags=%x\n",
						i, j_taskswitch->time, j_taskswitch->t_from, 
						j_taskswitch->t_to, str_wakeup_cause[wakeup_cause], 
						str_task_state[j_taskswitch->tfrom_state & 0x03], j_taskswitch->tfrom_flags);
				break;
				
			case JOURNAL_TYPE_IPCPOST:
				j_ipcop = &(journal_class1_history[i].ipcop);
				printf("record %02d: TTI=0x%08x IPCPOST %4s actint=%d tid=%d ipc_ptr=0x%x\n", 
						i, j_ipcop->time, str_ipc_type[(j_ipcop->ipctype-1)&0x03], 
						j_ipcop->active_ints, j_ipcop->tid, j_ipcop->ipc_ptr);
				break;
				
			case JOURNAL_TYPE_IPCPEND:
				j_ipcop = &(journal_class1_history[i].ipcop);
				printf("record %02d: TTI=0x%08x IPCPEND %4s actint=%d tid=%d ipc_ptr=0x%x\n", 
						i, j_ipcop->time, str_ipc_type[(j_ipcop->ipctype-1)&0x03], 
						j_ipcop->active_ints, j_ipcop->tid, j_ipcop->ipc_ptr);
				break;
				
			default:
				printf("unknown class1 type: %d\n", journal_class1_history[i].ipcop.jtype);
				break;
		}
	}
}

void crashdump_class2_journal(FILE *fp)
{
	char *str;
	char buf[256];
	int index=0, i;
	journal_class2_t journal_class2_history[JOURNAL_CLASS2_MAXRECORD];
	uint32_t *ptr1, *ptr2;
	struct journal_taskswitch *j_taskswitch;
	struct journal_ipcop *j_ipcop;
	
	// find "Class2 journal"
	while(str = fgets(buf, 256, fp))
	{
		str = strstr(buf, "Class2 journal");
		if(str) 
			break;
	}
	
	if(str == NULL)
	{
		printf("warning: can't find Class2 journal\n");
		return;
	}
	
	// read all records
	for(i=0; i<JOURNAL_CLASS2_MAXRECORD; i+=2)
	{
		str = fgets(buf, 256, fp);
		if(str == NULL) break;
		
		ptr1 = (uint32_t *)&journal_class2_history[i];
		ptr2 = (uint32_t *)&journal_class2_history[i+1];
		sscanf(str, " 0x%08x 0x%08x 0x%08x    0x%08x 0x%08x 0x%08x\n", &ptr1[0], &ptr1[1], &ptr1[2], &ptr2[0], &ptr2[1], &ptr2[2]);
	}
	
	// dump info
	for(i=0; i<JOURNAL_CLASS2_MAXRECORD; i++)
	{
		switch(journal_class2_history[i].tasklife.jtype)
		{
			case JOURNAL_TYPE_TASKCREATE:
			case JOURNAL_TYPE_TASKEXIT:
			case JOURNAL_TYPE_IPCINIT:
			case JOURNAL_TYPE_IPCDESTROY:
			default:
				break;
		}
	}
}

void crashdump_system_counter(FILE *fp)
{
}

void crashdump_task_counter(FILE *fp)
{
}

int main(int argc, char *argv[])
{
	FILE *fp;
	
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
	
	/* parser: Class1 journal */
	crashdump_class1_journal(fp);
	
	crashdump_class2_journal(fp);
	
	crashdump_system_counter(fp);
	
	crashdump_task_counter(fp);
	
	fclose(fp);
	
	return 0;
}
