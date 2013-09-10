/* samv_agent.h
 */
 
#ifndef _SAMV_AGENT_H_
#define _SAMV_AGENT_H_

#include "buffer.h"
#include "libsamv.h"

typedef struct samv_event_s
{
	uint8_t event;
	uint8_t port;
	uint8_t data1;
	uint8_t data2;
}samv_event_t;

typedef struct samv_mail_s
{
	uint8_t event;
	uint8_t port;
	uint16_t length;
	uint8_t *bufptr;
}samv_mail_t;

#define SAMV_MDATASIZE 64

enum{
/* STATE */
	//SAMVA_STATE_INIT = 0,
	SAMVA_STATE_FINDCD_S1 = 0,
	SAMVA_STATE_FINDCD_S2,
	SAMVA_STATE_SELECTCD_S1,
	SAMVA_STATE_SELECTCD_S2,
	SAMVA_STATE_READCD_S1,
	SAMVA_STATE_READCD_S2,

/* EVENT */
	SAMVA_EVENT_GERROR = 0,
	SAMVA_EVENT_TIMEOUT,
	SAMVA_EVENT_FINDCD,
	SAMVA_EVENT_SELECTCD,
	SAMVA_EVENT_READCD,
	SAMVA_EVENT_STATUS_QUERY,
	SAMVA_EVENT_READ_MINFO,
	SAMVA_EVENT_UNKNOWN_MSG,
	SAMVA_EVENT_RESPONSE,
/*
	SAMVA_EVENT_TARGET_ONLINE,
	SAMVA_EVENT_TARGET_OFFLINE,
	SAMVA_EVENT_HOST_ONLINE,
	SAMVA_EVENT_HOST_OFFLINE,
*/
	SAMVA_EVENT_HOSTMSG = 0x81,
	SAMVA_EVENT_TARGETMSG = 0x82,
};


typedef struct samv_agent
{
	uint8_t state;
	uint8_t port_alive[3];          // is port alive?
	uint8_t port_rxmsg[3];          // rx required message? for sync 
	uint32_t port_alive_trace[3];   // last alive time

	uint8_t samvid[16];
	uint8_t host_cmd[SAMV_MDATASIZE];
	uint8_t host_cmd_size;
	
	samv_rxbuf_t rxbuf[3];          // reassembling buffer for three ports
}samv_agent_t;

#endif
