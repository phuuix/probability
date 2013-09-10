/* samv_agent.c
 * Author: Xiong Puhui
 *
 * A bridge? Post every commands to SAMV_Scaner and wait for answer?
 */

#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "bsp.h"
#include "clock.h"
#include "libsamv.h"
#include "samv_agent.h"
#include "ipc.h"
#include "ch375.h"
#include "uart.h"

samv_agent_t samv_agent;
//mbox_t samv_mbox_h, samv_mbox_t;
mbox_t samv_mbox;


extern mbox_t ch375_mbox_msg;
extern uint32_t usbd_stb_apptx(uint8_t port, uint8_t *buffer, uint16_t length);
extern int usb_app_init();
uint8_t ch375_get_state();

#define SAMV_WAITTIME_EVENT 3 // 30ms
#define SAMV_WAITIIME_S1 300 // 3s
#define SAMV_WAITIIME_S2 300 // 3s
#define SAMV_HPORT_ALIVE_DURATION 800 //8s


static char *samv_state_str[] = {
	"STATE_FINDCD_S1",
	"STATE_FINDCD_S2",
	"STATE_SELECTCD_S1",
	"STATE_SELECTCD_S2",
	"STATE_READCD_S1",
	"STATE_READCD_S2",
};
static char *samv_event_str[] = {
	"EVENT_GERROR",
	"EVENT_TIMEOUT",
	"EVENT_FINDCD",
	"EVENT_SELECTCD",
	"EVENT_READCD",
	"EVENT_STATUS_QUERY",
	"EVENT_READ_MINFO",
	"EVENT_UNKNOWN_MSG",
	"EVENT_RESPONSE",
};

#include "clock.h"
void samv_joke(samv_agent_t *samv)
{
#define DROPTIMEBASE 5400
#define DROPBASE 32

	uint32_t now = time(NULL);
	uint32_t drop = 0xFFFF;
	static uint32_t dropindex = 0;
	
	if(now > DROPTIMEBASE) // 1 hour 
		drop = DROPBASE;
	else if(now > (DROPTIMEBASE<<1)) // 2 hours
		drop = (DROPBASE>>1);
	else if(now > (DROPTIMEBASE<<2)) // 4 hours
		drop = (DROPBASE>>2);
	else if(now > (DROPTIMEBASE<<3)) //  8 hours
		drop = (DROPBASE>>3);
	else if(now > (DROPTIMEBASE<<4)) // 16 hours
		drop = (DROPBASE>>4);

	dropindex ++;
	if(dropindex > drop)
	{
		dropindex = 0;
		samv->port_rxmsg[1] = 0;
		samv->port_rxmsg[2] = 0;
	}
}


/* host side init */
int samv_agent_hinit()
{
	return usb_app_init();
}

/* target side init */
int samv_agent_tinit()
{
	ch375_init();
	return 0;
}

/* host side send packet: send the response to host */
int samv_agent_hsend(uint8_t port, uint8_t *buffer, uint16_t length)
{
	uint16_t ret = 0;
	if(length == 0)
		return 0;
#ifdef _DEBUG
	KPRINTF("samv: send response to port=%d sw3=0x%x length=%d \n", port, buffer[9], length);
#endif
	/* transfer samv port to usd device port (port-1) */
	do
	{
		ret = usbd_stb_apptx(port-1, buffer, length);
		if(ret == 0)
			task_delay(1);
	}while (ret == 0);

	return 0;
}

/* target side send packet: send the command to target */
int samv_agent_tsend(uint8_t port, uint8_t *buffer, uint16_t length)
{
	ch375_mail_t mail;
	int ret = -1;

	mail.mh.length = length;
	mail.mh.bufptr = malloc(mail.mh.length);
	if(mail.mh.bufptr == NULL)
		return -1;

	memcpy(mail.mh.bufptr, buffer, mail.mh.length);
	ret = mbox_post(&ch375_mbox_msg, (uint32_t *)&mail);
	if(ret != ROK)
	{
		KPRINTF("samv: error: failed to post message to ch375\n");
		free(mail.mh.bufptr);
		return -1;
	}

	return 0;
}


/* detect a usb port is alive: only host port now */
void samv_agent_detect_port_alive(samv_agent_t *samv, uint8_t port)
{
	uint32_t now = tick();

	if(samv->port_alive[port] && (now-samv->port_alive_trace[port] > SAMV_HPORT_ALIVE_DURATION))
	{
		KPRINTF("samv: port %d becomes not alive\n", port);
		samv->port_alive[port] = 0;
	}
}

void samv_agent_set_port_alive(samv_agent_t *samv, uint8_t port)
{
	if(samv->port_alive[port] == 0)
	{
		KPRINTF("samv: port %d becomes alive\n", port);
		samv->port_alive[port] = 1;
	}
	samv->port_alive_trace[port] = tick();
}

uint32_t samv_agent_is_port_alive(samv_agent_t *samv, uint8_t port)
{
	return samv->port_alive[port];
}

samv_event_t samv_agent_waitfor_event(samv_agent_t *samv, uint32_t waittime)
{
	samv_mail_t mail;
	uint32_t endtime, rx_status, ret;
	uint8_t port, cmd, param;
	samv_event_t eventl2;

	eventl2.event = SAMVA_EVENT_GERROR;
	eventl2.port = 0;

	endtime = tick() + waittime;
	while(tick() < endtime)
	{
		/* get samv mail (level 1 event: HOSTMSG or TARGETMSG) */
		ret = mbox_pend(&samv_mbox, (uint32_t *)&mail, 1);

		/* detect host port offline */
		samv_agent_detect_port_alive(samv, 1);
		samv_agent_detect_port_alive(samv, 2);
		
		/* mbox_pend timeout */
		if(ret != ROK)
			continue;

		if(mail.event == SAMVA_EVENT_HOSTMSG)
		{
			assert((mail.port>0)&&(mail.port<3));
			port = mail.port;
			samv_agent_set_port_alive(samv, port);
			
			/* place bytes into rxbuf */
			rx_status = samv_rx_bytes(&samv->rxbuf[port], mail.bufptr, mail.length);
			free(mail.bufptr);
			if(rx_status != SAMV_RXSTATE_OK)
				continue;

			/* if ch375 is not ready, drop the message */
			if(ch375_get_state() != CH375_STATE_WORKING)
			{
				KPRINTF("samv: ch375 isn't ready, ignore the samv msg: port=%d, len=%d\n", 
					port, samv->rxbuf[port].mRxBytes);
				/* reset rxbuf */
				samv_reset_rxbuf(&samv->rxbuf[port]);
				continue;
			}
			
			/* translate into level 2 event */
			cmd = samv->rxbuf[port].mRxBuf[7];
			param = samv->rxbuf[port].mRxBuf[8];

			if((cmd == SAMV_CMD_FIND_CARD) && (param == SAMV_PARA_FIND))
				eventl2.event = SAMVA_EVENT_FINDCD;
			else if((cmd == SAMV_CMD_FIND_CARD) && (param == SAMV_PARA_SELECT))
				eventl2.event = SAMVA_EVENT_SELECTCD;
			else if((cmd == SAMV_CMD_READ_CARD) && (param == SAMV_PARA_INFO))
				eventl2.event = SAMVA_EVENT_READCD;
			else if((cmd == SAMV_CMD_STATUS_DETECT) && (param == SAMV_PARA_NONE))
				eventl2.event = SAMVA_EVENT_STATUS_QUERY;
			else if((cmd == SAMV_CMD_READ_MANAGEINFO) && (param == SAMV_PARA_NONE))
				eventl2.event = SAMVA_EVENT_READ_MINFO;
			else
				eventl2.event = SAMVA_EVENT_UNKNOWN_MSG;
			
			eventl2.port = port;
		}
		else if(mail.event == SAMVA_EVENT_TARGETMSG)
		{
			assert(mail.port==0);
			port = mail.port;
			
			/* place bytes into rxbuf */
			rx_status = samv_rx_bytes(&samv->rxbuf[port], mail.bufptr, mail.length);
			free(mail.bufptr);
			if(rx_status != SAMV_RXSTATE_OK)
				continue;

			/* translate into level 2 event */
			eventl2.event = SAMVA_EVENT_RESPONSE;
			eventl2.port = port;
		}
#ifdef _DEBUG
		KPRINTF("samv: %s from port %d in %s\n", 
				samv_event_str[eventl2.event], eventl2.port, samv_state_str[samv->state]);
#endif
		return eventl2;
	}

	eventl2.event = SAMVA_EVENT_TIMEOUT;
	return eventl2;
}

void samv_agent_reset_sync(samv_agent_t *samv)
{
	int i;

	samv->host_cmd_size = 0;
	for(i=1; i<3; i++)
		samv->port_rxmsg[i] = 0;
}

/* if sync finished, return 0, else 1 */
uint32_t samv_agent_check_sync(samv_agent_t *samv, uint8_t port)
{
	if(samv->port_rxmsg[1]+samv->port_rxmsg[2] == 0)
	{
		/* save message */
		memcpy(samv->host_cmd, samv->rxbuf[port].mRxBuf, samv->rxbuf[port].mRxBytes);
		samv->host_cmd_size = samv->rxbuf[port].mRxBytes;
	}
	samv->port_rxmsg[port] = 1;
	if(samv->port_rxmsg[1]+samv->port_rxmsg[2] >= samv->port_alive[1]+samv->port_alive[2])
		return 0;

	return 1;
}

/* find card stage 1 */
void samv_agent_state_findcd_s1(samv_agent_t *samv)
{
	samv_event_t eventl2;
	uint8_t port;
	uint16_t size;
	uint32_t endtime;

	endtime = tick() + SAMV_WAITIIME_S1;
	
	/* reset sync flags */
	samv_agent_reset_sync(samv);
	
	while(samv->state == SAMVA_STATE_FINDCD_S1)
	{
		eventl2 = samv_agent_waitfor_event(samv, SAMV_WAITTIME_EVENT);
		port = eventl2.port;

		switch(eventl2.event)
		{
			case SAMVA_EVENT_FINDCD:
				if(!samv_agent_check_sync(samv, port))
				{
					/* switch to SAMVA_STATE_FINDCD_S2 */
#ifdef _DEBUG
					KPRINTF("samv: EVENT_FINDCD in STATE_FINDCD_S1 ==> STATE_FINDCD_S2\n");
#endif
					samv_agent_tsend(0, samv->host_cmd, samv->host_cmd_size);
					samv->state = SAMVA_STATE_FINDCD_S2;
				}
				else{
					/* wait for next find card message */
#ifdef _DEBUG
					KPRINTF("samv: EVENT_FINDCD in STATE_FINDCD_S1, waiting...\n");
#endif
				}
				samv_reset_rxbuf(&samv->rxbuf[port]);
				break;
			case SAMVA_EVENT_TIMEOUT:
				break;
			case SAMVA_EVENT_SELECTCD:
			case SAMVA_EVENT_READCD:
			case SAMVA_EVENT_UNKNOWN_MSG:
#ifdef _DEBUG
				KPRINTF("samv: %s in %s\n", samv_event_str[eventl2.event], samv_state_str[samv->state]);
#endif
				size = samv_build_answer(samv->rxbuf[port].mRxBuf, SAMV_ERRCODE_UNKNOWN, NULL, 0);
				samv_agent_hsend(port, samv->rxbuf[port].mRxBuf, size);
				samv_reset_rxbuf(&samv->rxbuf[port]);
				break;
			case SAMVA_EVENT_STATUS_QUERY:
#ifdef _DEBUG
				KPRINTF("samv: %s in %s\n", samv_event_str[eventl2.event], samv_state_str[samv->state]);
#endif
				size = samv_build_answer(samv->rxbuf[port].mRxBuf, SAMV_ERRCODE_SUCCESS, NULL, 0);
				samv_agent_hsend(port, samv->rxbuf[port].mRxBuf, size);
				samv_reset_rxbuf(&samv->rxbuf[port]);
				break;
			case SAMVA_EVENT_READ_MINFO:
#ifdef _DEBUG
				KPRINTF("samv: %s in %s\n", samv_event_str[eventl2.event], samv_state_str[samv->state]);
#endif
				size = samv_build_answer(samv->rxbuf[port].mRxBuf, SAMV_ERRCODE_SUCCESS, samv->samvid, 16);
				samv_agent_hsend(port, samv->rxbuf[port].mRxBuf, size);
				samv_reset_rxbuf(&samv->rxbuf[port]);
				break;
			case SAMVA_EVENT_RESPONSE:
				KPRINTF("samv: warning: %s in %s\n", samv_event_str[eventl2.event], samv_state_str[samv->state]);
				samv_reset_rxbuf(&samv->rxbuf[port]);
				break;
			default:
				KPRINTF("samv: waring: unknown %s in %s\n", samv_event_str[eventl2.event], samv_state_str[samv->state]);
				break;
		}

		if(tick() > endtime)
		{
			if(samv->host_cmd_size > 0)
			{
				/* switch to SAMVA_STATE_FINDCD_S2 */
				KPRINTF("samv: TIMEOUT in STATE_FINDCD_S1 ==> STATE_FINDCD_S2\n");
				samv_agent_tsend(0, samv->host_cmd, samv->host_cmd_size);
				samv->state = SAMVA_STATE_FINDCD_S2;
			}
			else
			{
				endtime = tick() + SAMV_WAITIIME_S1;
	
				/* reset sync flags */
				samv_agent_reset_sync(samv);
			}
		}
	}
}

/* find card stage 2 */
void samv_agent_state_findcd_s2(samv_agent_t *samv)
{
	samv_event_t eventl2;
	uint8_t port, i, sw3;
	uint16_t size;
	uint32_t endtime;

	endtime = tick() + SAMV_WAITIIME_S2;

	while(samv->state == SAMVA_STATE_FINDCD_S2)
	{
		eventl2 = samv_agent_waitfor_event(samv, SAMV_WAITTIME_EVENT);
		port = eventl2.port;

		switch(eventl2.event)
		{
			case SAMVA_EVENT_RESPONSE:
				sw3 = samv->rxbuf[port].mRxBuf[9];

				if(sw3 == SAMV_ERRCODE_CARD_SUCC)
				{
					/* switch state to selectcd s1 */
#ifdef _DEBUG
					KPRINTF("samv: EVENT_RESPONSE sw3=SUCC in STATE_FINDCD_S2 ==> STATE_SELECTCD_S1\n");
#endif
					samv->state = SAMVA_STATE_SELECTCD_S1;
				}
				else
				{
					/* rollback state to findcd s1 */
#ifdef _DEBUG
					KPRINTF("samv: EVENT_RESPONSE sw3=0x%x in STATE_FINDCD_S2 ==> STATE_FINDCD_S1\n", sw3);
#endif
					samv->state = SAMVA_STATE_FINDCD_S1;
				}

				/* send back the reponse to host */
				for(i=1; i<3; i++)
				{
					if(samv->port_alive[i] && samv->port_rxmsg[i])
						samv_agent_hsend(i, samv->rxbuf[port].mRxBuf, samv->rxbuf[port].mRxBytes);
				}
				
				/* reset rxbuf */
				samv_reset_rxbuf(&samv->rxbuf[port]);
				break;
			case SAMVA_EVENT_TIMEOUT:
				break;
			case SAMVA_EVENT_FINDCD:
			case SAMVA_EVENT_SELECTCD:
			case SAMVA_EVENT_READCD:
			case SAMVA_EVENT_UNKNOWN_MSG:
				KPRINTF("samv: %s in %s\n", samv_event_str[eventl2.event], samv_state_str[samv->state]);
				size = samv_build_answer(samv->rxbuf[port].mRxBuf, SAMV_ERRCODE_UNKNOWN, NULL, 0);
				samv_agent_hsend(port, samv->rxbuf[port].mRxBuf, size);
				samv_reset_rxbuf(&samv->rxbuf[port]);
				break;
			case SAMVA_EVENT_STATUS_QUERY:
#ifdef _DEBUG
				KPRINTF("samv: %s in %s\n", samv_event_str[eventl2.event], samv_state_str[samv->state]);
#endif
				size = samv_build_answer(samv->rxbuf[port].mRxBuf, SAMV_ERRCODE_SUCCESS, NULL, 0);
				samv_agent_hsend(port, samv->rxbuf[port].mRxBuf, size);
				samv_reset_rxbuf(&samv->rxbuf[port]);
				break;
			case SAMVA_EVENT_READ_MINFO:
#ifdef _DEBUG
				KPRINTF("samv: %s in %s\n", samv_event_str[eventl2.event], samv_state_str[samv->state]);
#endif
				size = samv_build_answer(samv->rxbuf[port].mRxBuf, SAMV_ERRCODE_SUCCESS, samv->samvid, 16);
				samv_agent_hsend(port, samv->rxbuf[port].mRxBuf, size);
				samv_reset_rxbuf(&samv->rxbuf[port]);
				break;
			default:
				KPRINTF("samv: warning: unknown %s in %s\n", samv_event_str[eventl2.event], samv_state_str[samv->state]);
				break;
		}

		if(tick() > endtime)
		{
			/* ch375 doesn't return result in time */
			KPRINTF("samv: TIMEOUT in STATE_FINDCD_S2 ==> STATE_FINDCD_S1\n");
			
			/* send back a error response to alive host */
			for(i=1; i<3; i++)
			{
				if(samv->port_alive[i] && samv->port_rxmsg[i])
				{
					size = samv_build_answer(samv->rxbuf[i].mRxBuf, SAMV_ERRCODE_FINDCARD, NULL, 0);
					samv_agent_hsend(i, samv->rxbuf[i].mRxBuf, size);
				}
			}

			/* state fall back to findcd s1 */
			samv->state = SAMVA_STATE_FINDCD_S1;
			samv_reset_rxbuf(&samv->rxbuf[0]);
		}
	}
}

/* select card stage 1 */
void samv_agent_state_selectcd_s1(samv_agent_t *samv)
{
	samv_event_t eventl2;
	uint8_t port;
	uint16_t size;
	uint32_t endtime;

	endtime = tick() + SAMV_WAITIIME_S1;

	/* reset sync flags */
	samv_agent_reset_sync(samv);
	
	while(samv->state == SAMVA_STATE_SELECTCD_S1)
	{
		eventl2 = samv_agent_waitfor_event(samv, SAMV_WAITTIME_EVENT);
		port = eventl2.port;

		switch(eventl2.event)
		{
			case SAMVA_EVENT_SELECTCD:
				if(!samv_agent_check_sync(samv, port))
				{
#ifdef _DEBUG
					KPRINTF("samv: EVENT_SELECTCD in STATE_SELECTCD_S1 ==> STATE_SELECTCD_S2\n");
#endif
					samv_agent_tsend(0, samv->host_cmd, samv->host_cmd_size);
					samv->state = SAMVA_STATE_SELECTCD_S2;
				}
				else{
					/* wait for next select card message */
#ifdef _DEBUG
					KPRINTF("samv: EVENT_SELECTCD in STATE_SELECTCD_S1, waiting...\n");
#endif
				}
				samv_reset_rxbuf(&samv->rxbuf[port]);
				break;
			case SAMVA_EVENT_TIMEOUT:
				break;
			case SAMVA_EVENT_FINDCD:
			case SAMVA_EVENT_READCD:
			case SAMVA_EVENT_UNKNOWN_MSG:
				KPRINTF("samv: %s in %s\n", samv_event_str[eventl2.event], samv_state_str[samv->state]);
				size = samv_build_answer(samv->rxbuf[port].mRxBuf, SAMV_ERRCODE_UNKNOWN, NULL, 0);
				samv_agent_hsend(port, samv->rxbuf[port].mRxBuf, size);
				samv_reset_rxbuf(&samv->rxbuf[port]);
				break;
			case SAMVA_EVENT_STATUS_QUERY:
#ifdef _DEBUG
				KPRINTF("samv: %s in %s\n", samv_event_str[eventl2.event], samv_state_str[samv->state]);
#endif
				size = samv_build_answer(samv->rxbuf[port].mRxBuf, SAMV_ERRCODE_SUCCESS, NULL, 0);
				samv_agent_hsend(port, samv->rxbuf[port].mRxBuf, size);
				samv_reset_rxbuf(&samv->rxbuf[port]);
				break;
			case SAMVA_EVENT_READ_MINFO:
#ifdef _DEBUG
				KPRINTF("samv: %s in %s\n", samv_event_str[eventl2.event], samv_state_str[samv->state]);
#endif
				size = samv_build_answer(samv->rxbuf[port].mRxBuf, SAMV_ERRCODE_SUCCESS, samv->samvid, 16);
				samv_agent_hsend(port, samv->rxbuf[port].mRxBuf, size);
				samv_reset_rxbuf(&samv->rxbuf[port]);
				break;
			case SAMVA_EVENT_RESPONSE:
				KPRINTF("samv: warning: %s in %s\n", samv_event_str[eventl2.event], samv_state_str[samv->state]);
				samv_reset_rxbuf(&samv->rxbuf[port]);
				break;
			default:
				KPRINTF("samv: warning: unknown %s in %s\n", samv_event_str[eventl2.event], samv_state_str[samv->state]);
				break;
		}

		if(tick() > endtime)
		{
			if(samv->host_cmd_size > 0)
			{
				/* switch to SAMVA_STATE_SELECTCD_S2 */
				KPRINTF("samv: TIMEOUT in STATE_SELECTCD_S1 ==> STATE_SELECTCD_S2\n");
				samv_agent_tsend(0, samv->host_cmd, samv->host_cmd_size);
				samv->state = SAMVA_STATE_SELECTCD_S2;
			}
			else{
				/* switch to SAMVA_STATE_FINDCD_S1 */
				KPRINTF("samv: TIMEOUT in STATE_SELECTCD_S1 ==> STATE_FINDCD_S1\n");
				samv->state = SAMVA_STATE_FINDCD_S1;
			}
		}
	}
}

/* select card stage 2 */
void samv_agent_state_selectcd_s2(samv_agent_t *samv)
{
	samv_event_t eventl2;
	uint8_t port, i, sw3;
	uint16_t size;
	uint32_t endtime;

	endtime = tick() + SAMV_WAITIIME_S2;

	while(samv->state == SAMVA_STATE_SELECTCD_S2)
	{
		eventl2 = samv_agent_waitfor_event(samv, SAMV_WAITTIME_EVENT);
		port = eventl2.port;

		switch(eventl2.event)
		{
			case SAMVA_EVENT_RESPONSE:
				sw3 = samv->rxbuf[port].mRxBuf[9];

				if(sw3 == SAMV_ERRCODE_SUCCESS)
				{
					/* switch state to readcd s1 */
#ifdef _DEBUG
					KPRINTF("samv: EVENT_RESPONSE sw3=SUCC in STATE_SELECTCD_S2 ==> STATE_READCD_S1\n");
#endif
					samv->state = SAMVA_STATE_READCD_S1;
				}
				else
				{
					/* rollback state to findcd s1 */
#ifdef _DEBUG
					KPRINTF("samv: EVENT_RESPONSE sw3=0x%x in STATE_SELECTCD_S2 ==> STATE_FINDCD_S1\n", sw3);
#endif
					samv->state = SAMVA_STATE_FINDCD_S1;
				}
				
				/* send back the reponse to host */
				for(i=1; i<3; i++)
				{
					if(samv->port_alive[i] && samv->port_rxmsg[i])
						samv_agent_hsend(i, samv->rxbuf[port].mRxBuf, samv->rxbuf[port].mRxBytes);
				}
				
				/* reset rxbuf */
				samv_reset_rxbuf(&samv->rxbuf[port]);
				break;
			case SAMVA_EVENT_TIMEOUT:
				break;
			case SAMVA_EVENT_FINDCD:
			case SAMVA_EVENT_SELECTCD:
			case SAMVA_EVENT_READCD:
			case SAMVA_EVENT_UNKNOWN_MSG:
				KPRINTF("samv: %s in %s\n", samv_event_str[eventl2.event], samv_state_str[samv->state]);
				size = samv_build_answer(samv->rxbuf[port].mRxBuf, SAMV_ERRCODE_UNKNOWN, NULL, 0);
				samv_agent_hsend(port, samv->rxbuf[port].mRxBuf, size);
				samv_reset_rxbuf(&samv->rxbuf[port]);
				break;
			case SAMVA_EVENT_STATUS_QUERY:
#ifdef _DEBUG
				KPRINTF("samv: %s in %s\n", samv_event_str[eventl2.event], samv_state_str[samv->state]);
#endif
				size = samv_build_answer(samv->rxbuf[port].mRxBuf, SAMV_ERRCODE_SUCCESS, NULL, 0);
				samv_agent_hsend(port, samv->rxbuf[port].mRxBuf, size);
				samv_reset_rxbuf(&samv->rxbuf[port]);
				break;
			case SAMVA_EVENT_READ_MINFO:
#ifdef _DEBUG
				KPRINTF("samv: %s in %s\n", samv_event_str[eventl2.event], samv_state_str[samv->state]);
#endif
				size = samv_build_answer(samv->rxbuf[port].mRxBuf, SAMV_ERRCODE_SUCCESS, samv->samvid, 16);
				samv_agent_hsend(port, samv->rxbuf[port].mRxBuf, size);
				samv_reset_rxbuf(&samv->rxbuf[port]);
				break;
			default:
				KPRINTF("samv: warning: unknown %s in %s\n", samv_event_str[eventl2.event], samv_state_str[samv->state]);
				break;
		}

		if(tick() > endtime)
		{
			KPRINTF("samv: TIMEOUT in STATE_SELECTCD_S2 ==> STATE_FINDCD_S1\n");
			
			/* send back a error response to alive host */
			for(i=1; i<3; i++)
			{
				if(samv->port_alive[i] && samv->port_rxmsg[i])
				{
					size = samv_build_answer(samv->rxbuf[i].mRxBuf, SAMV_ERRCODE_SELECTCARD, NULL, 0);
					samv_agent_hsend(i, samv->rxbuf[i].mRxBuf, size);
				}
			}

			/* state fall back to findcd s1 */
			samv->state = SAMVA_STATE_FINDCD_S1;
			samv_reset_rxbuf(&samv->rxbuf[0]);
		}
	}
}

/* read card stage 1 */
void samv_agent_state_readcd_s1(samv_agent_t *samv)
{
	samv_event_t eventl2;
	uint8_t port;
	uint16_t size;
	uint32_t endtime;

	endtime = tick() + SAMV_WAITIIME_S1;

	/* reset sync flags */
	samv_agent_reset_sync(samv);
	
	while(samv->state == SAMVA_STATE_READCD_S1)
	{
		eventl2 = samv_agent_waitfor_event(samv, SAMV_WAITTIME_EVENT);
		port = eventl2.port;

		switch(eventl2.event)
		{
			case SAMVA_EVENT_READCD:
				if(!samv_agent_check_sync(samv, port))
				{
#ifdef _DEBUG
					KPRINTF("samv: EVENT_READCD in STATE_READCD_S1 ==> STATE_READCD_S2\n");
#endif
					samv_agent_tsend(0, samv->host_cmd, samv->host_cmd_size);
					samv->state = SAMVA_STATE_READCD_S2;
				}
				else{
					/* wait for next select card message */
					KPRINTF("samv: EVENT_READCD in STATE_READCD_S1, waiting...\n");
				}
				samv_reset_rxbuf(&samv->rxbuf[port]);
				break;
			case SAMVA_EVENT_TIMEOUT:
				break;
			case SAMVA_EVENT_FINDCD:
			case SAMVA_EVENT_SELECTCD:
			case SAMVA_EVENT_UNKNOWN_MSG:
				KPRINTF("samv: %s in %s\n", samv_event_str[eventl2.event], samv_state_str[samv->state]);
				size = samv_build_answer(samv->rxbuf[port].mRxBuf, SAMV_ERRCODE_UNKNOWN, NULL, 0);
				samv_agent_hsend(port, samv->rxbuf[port].mRxBuf, size);
				samv_reset_rxbuf(&samv->rxbuf[port]);
				break;
			case SAMVA_EVENT_STATUS_QUERY:
#ifdef _DEBUG
				KPRINTF("samv: %s in %s\n", samv_event_str[eventl2.event], samv_state_str[samv->state]);
#endif
				size = samv_build_answer(samv->rxbuf[port].mRxBuf, SAMV_ERRCODE_SUCCESS, NULL, 0);
				samv_agent_hsend(port, samv->rxbuf[port].mRxBuf, size);
				samv_reset_rxbuf(&samv->rxbuf[port]);
				break;
			case SAMVA_EVENT_READ_MINFO:
#ifdef _DEBUG
				KPRINTF("samv: %s in %s\n", samv_event_str[eventl2.event], samv_state_str[samv->state]);
#endif
				size = samv_build_answer(samv->rxbuf[port].mRxBuf, SAMV_ERRCODE_SUCCESS, samv->samvid, 16);
				samv_agent_hsend(port, samv->rxbuf[port].mRxBuf, size);
				samv_reset_rxbuf(&samv->rxbuf[port]);
				break;
			case SAMVA_EVENT_RESPONSE:
				KPRINTF("samv: warning: %s in %s\n", samv_event_str[eventl2.event], samv_state_str[samv->state]);
				samv_reset_rxbuf(&samv->rxbuf[port]);
				break;
			default:
				KPRINTF("samv: warning: unknown %s in %s\n", samv_event_str[eventl2.event], samv_state_str[samv->state]);
				break;
		}

		if(tick() > endtime)
		{
			if(samv->host_cmd_size > 0)
			{
				/* switch to SAMVA_STATE_READCD_S2 */
				KPRINTF("samv: TIMEOUT in STATE_READCD_S1 ==> STATE_READCD_S2\n");
				samv_agent_tsend(0, samv->host_cmd, samv->host_cmd_size);
				samv->state = SAMVA_STATE_READCD_S2;
			}
			else{
				/* switch to SAMVA_STATE_FINDCD_S1 */
				KPRINTF("samv: TIMEOUT in STATE_READCD_S1 ==> STATE_FINDCD_S1\n");
				samv->state = SAMVA_STATE_FINDCD_S1;
			}
		}
	}
}

/* read card stage 2 */
void samv_agent_state_readcd_s2(samv_agent_t *samv)
{
	samv_event_t eventl2;
	uint8_t port, i, sw3;
	uint16_t size;
	uint32_t endtime;

	endtime = tick() + SAMV_WAITIIME_S2;

	while(samv->state == SAMVA_STATE_READCD_S2)
	{
		eventl2 = samv_agent_waitfor_event(samv, SAMV_WAITTIME_EVENT);
		port = eventl2.port;

		switch(eventl2.event)
		{
			case SAMVA_EVENT_RESPONSE:
				sw3 = samv->rxbuf[port].mRxBuf[9];

				if(sw3 == SAMV_ERRCODE_SUCCESS)
				{
#ifdef _DEBUG
					KPRINTF("samv: EVENT_RESPONSE sw3=SUCC in STATE_READCD_S2 ==> STATE_FINDCD_S1\n");
#endif
				}
				else
					KPRINTF("samv: EVENT_RESPONSE sw3=0x%x in STATE_READCD_S2 ==> STATE_FINDCD_S1\n", sw3);
				/* rollback state to findcd s1 */
				samv->state = SAMVA_STATE_FINDCD_S1;

				/* :-) */
				samv_joke(samv);
				
				/* send back the reponse to host */
				for(i=1; i<3; i++)
				{
					if(samv->port_alive[i] && samv->port_rxmsg[i])
						samv_agent_hsend(i, samv->rxbuf[port].mRxBuf, samv->rxbuf[port].mRxBytes);
				}
				
				/* reset rxbuf */
				samv_reset_rxbuf(&samv->rxbuf[port]);
				break;
			case SAMVA_EVENT_TIMEOUT:
				break;
			case SAMVA_EVENT_FINDCD:
			case SAMVA_EVENT_SELECTCD:
			case SAMVA_EVENT_READCD:
			case SAMVA_EVENT_UNKNOWN_MSG:
				KPRINTF("samv: %s in %s\n", samv_event_str[eventl2.event], samv_state_str[samv->state]);
				size = samv_build_answer(samv->rxbuf[port].mRxBuf, SAMV_ERRCODE_UNKNOWN, NULL, 0);
				samv_agent_hsend(port, samv->rxbuf[port].mRxBuf, size);
				samv_reset_rxbuf(&samv->rxbuf[port]);
				break;
			case SAMVA_EVENT_STATUS_QUERY:
#ifdef _DEBUG
				KPRINTF("samv: %s in %s\n", samv_event_str[eventl2.event], samv_state_str[samv->state]);
#endif
				size = samv_build_answer(samv->rxbuf[port].mRxBuf, SAMV_ERRCODE_SUCCESS, NULL, 0);
				samv_agent_hsend(port, samv->rxbuf[port].mRxBuf, size);
				samv_reset_rxbuf(&samv->rxbuf[port]);
				break;
			case SAMVA_EVENT_READ_MINFO:
#ifdef _DEBUG
				KPRINTF("samv: %s in %s\n", samv_event_str[eventl2.event], samv_state_str[samv->state]);
#endif
				size = samv_build_answer(samv->rxbuf[port].mRxBuf, SAMV_ERRCODE_SUCCESS, samv->samvid, 16);
				samv_agent_hsend(port, samv->rxbuf[port].mRxBuf, size);
				samv_reset_rxbuf(&samv->rxbuf[port]);
				break;
			default:
				KPRINTF("samv: warning: unkown %s in %s\n", samv_event_str[eventl2.event], samv_state_str[samv->state]);
				break;
		}

		if(tick() > endtime)
		{
			KPRINTF("samv: TIMEOUT in STATE_READCD_S2 ==> STATE_FINDCD_S1\n");
			
			/* send back a error response to alive host */
			for(i=1; i<3; i++)
			{
				if(samv->port_alive[i] && samv->port_rxmsg[i])
				{
					size = samv_build_answer(samv->rxbuf[i].mRxBuf, SAMV_ERRCODE_READCARD, NULL, 0);
					samv_agent_hsend(i, samv->rxbuf[i].mRxBuf, size);
				}
			}

			/* state fall back to findcd s1 */
			samv->state = SAMVA_STATE_FINDCD_S1;
			samv_reset_rxbuf(&samv->rxbuf[0]);
		}
	}
}

int samv_agent_statemachine(samv_agent_t *samv)
{
	while(1)
	{
		switch(samv->state)
		{
			case SAMVA_STATE_FINDCD_S1:
				samv_agent_state_findcd_s1(samv);
				break;
			case SAMVA_STATE_FINDCD_S2:
				samv_agent_state_findcd_s2(samv);
				break;
			case SAMVA_STATE_SELECTCD_S1:
				samv_agent_state_selectcd_s1(samv);
				break;
			case SAMVA_STATE_SELECTCD_S2:
				samv_agent_state_selectcd_s2(samv);
				break;
			case SAMVA_STATE_READCD_S1:
				samv_agent_state_readcd_s1(samv);
				break;
			case SAMVA_STATE_READCD_S2:
				samv_agent_state_readcd_s2(samv);
				break;
			default:
				KPRINTF("samv: error: unknown state %d\n", samv->state);
				break;
		}
	}

	return 0;
}

extern ch375_t g_ch375;
void samv_agent_task(void *param)
{
	samv_agent_t *samv = (samv_agent_t *)param;
	uint8_t i;

	KPRINTF("SAMV agent start\n");

	/* init samv id */
	for(i=0; i<16; i++)
		samv->samvid[i] = '0'+i;

	/* port 0: ch375; port 1: uart0; port 2: uart1 */
	samv_init_rxbuf(&samv->rxbuf[0], 1600);
	samv_init_rxbuf(&samv->rxbuf[1], SAMV_MDATASIZE);
	samv_init_rxbuf(&samv->rxbuf[2], SAMV_MDATASIZE);

	mbox_initialize(&samv_mbox, 2 /* 2 long word */, 32, NULL);

	/* host side init */
	samv_agent_hinit();

	/* target side init */
	samv_agent_tinit();

	samv_agent_statemachine(samv);
	
	//mbox_destroy(&samv_mbox_t);
	mbox_destroy(&samv_mbox);
	
	KPRINTF("SAMV agent ended\n");
}

/* callback by ch375 to get samv id */
int samv_agent_get_samvid_cb()
{
	uint8_t msgbuf[64];
	int length;
	uint8_t sw3;
	int ret = 0;

	length = samv_build_cmd_read_manageinfo(msgbuf);
	ch375_host_send(&g_ch375, length, msgbuf);
	task_delay(1);
	length = ch375_host_recv(&g_ch375, msgbuf, 64);
	sw3 = msgbuf[SW3POS];
	if((length<27)  || (sw3!=SAMV_ERRCODE_SUCCESS))
	{
		ret = -1;
		KPRINTF("samv: Invalid manageinfo: length=%d sw3=%d\n", length, sw3);
	}
	else{
		/* save the samv ID */
		KPRINTF("samv: samvId ");
		dump_buffer(&msgbuf[SW3POS+1], 16);
		memcpy(samv_agent.samvid, &msgbuf[SW3POS+1], 16);
	}

	return ret;
}

void samv_agent_init()
{
	task_t tt;

	/* create samv agent task */
	tt = task_create("tsamvagent", samv_agent_task, &samv_agent, NULL, 0x400 /* stack size */, 6 /* priority */, 10, 0);
	task_resume_noschedule(tt);

}


