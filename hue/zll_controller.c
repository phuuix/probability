/*
 * zll_controller.c
 *
 * A stateless ZLL controller
 *
 */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "ipc.h"
#include "uprintf.h"
#include "clock.h"
#include "ptimer.h"
#include "assert.h"
#include "hue.h"
#include "zll_controller.h"
#include "zllSocCmd.h"

mbox_t g_hue_mbox;  // Rx message from HTTP or console
mbox_t g_zll_mbox;  // Rx message from ZLL

hue_t g_hue;
ptimer_table_t g_zll_timer_table;


static hue_mail_t hue_mbox_data[HUE_MBOX_SIZE];
static zll_mail_t zll_mbox_data[ZLL_MBOX_SIZE];
static char zll_task_stack[TZLL_STACK_SIZE];

#define MAX_CONSOLE_CMD_LEN 128

void commandUsage( void );
void zllctrl_process_console_command( char *cmdBuff );
void getConsoleCommandParams(char* cmdBuff, uint16_t *nwkAddr, uint8_t *addrMode, uint8_t *ep, uint8_t *value, uint16_t *transitionTime);
uint32_t getParam( char *cmdBuff, char *paramId, uint32_t *paramInt);
uint8_t tlIndicationCb(epInfo_t *epInfo);
uint8_t newDevIndicationCb(epInfo_t *epInfo);

static uint16_t savedNwkAddr;    
static uint8_t savedAddrMode;    
static uint8_t savedEp;    
static uint8_t savedValue;    
static uint16_t savedTransitionTime; 

void commandUsage( void )
{
    uprintf_default("Commands:\n");
    uprintf_default("touchlink\n");
    uprintf_default("sendresettofn\n"); 
    uprintf_default("resettofn\n"); 
    uprintf_default("open\n"); 
    uprintf_default("setstate\n");
    uprintf_default("setlevel\n"); 
    uprintf_default("sethue\n");
    uprintf_default("setsat\n");
    uprintf_default("getstate\n"); 
    uprintf_default("getlevel\n"); 
    uprintf_default("gethue\n"); 
    uprintf_default("getsat\n\n");    
    uprintf_default("exit\n\n");
    
    uprintf_default("Parameters:\n");
    uprintf_default("-n<network address>\n");    
    uprintf_default("-e<end point>\n");
    uprintf_default("-m<address mode 1=groupcast, 2=unicast>\n");
    uprintf_default("-v<value>\n");
    uprintf_default("-t<transition time in 10ms>\n\n");
    
    uprintf_default("A successful touchlink will set the network address and endpoint of the newly\n");
    uprintf_default("paired device. so all the new Light can be turn on with the following command\n");
    uprintf_default("setsat -v1\n\n");    
    
    uprintf_default("The below example  will send a MoveToHue command to network address 0x0003\n");
    uprintf_default("end point 0xb, which will cause the device to move to a red hue over 3s\n");
    uprintf_default("(be sure that the saturation is set high to see the change):\n");
    uprintf_default("sethue -n0x0003 -e0xb -m0x2 -v0 -t30\n\n");
    
    uprintf_default("parameters are remembered, so the blow command directly after the above will\n");
    uprintf_default("change to a blue hue:\n");    
    uprintf_default("sethue -v0xAA \n\n");
    
    uprintf_default("The value of hue is a 0x0-0xFF representation of the 360Deg color wheel where:\n");
    uprintf_default("0Deg or 0x0 is red\n");
    uprintf_default("120Deg or 0x55 is a green hue\n");
    uprintf_default("240Deg or 0xAA is a blue hue\n\n");
    
    uprintf_default("The value of saturation is a 0x0-0xFE value where:\n");
    uprintf_default("0x0 is white\n");
    uprintf_default("0xFE is the fully saturated color specified by the hue value\n");        
}

int zllctrl_post_console_command(char *command)
{
    hue_mail_t huemail;

    huemail.console.cmd = 0;
    huemail.console.length = strlen(command);
    huemail.console.data = (uint8_t *)command;

    return mbox_post(&g_hue_mbox, (uint32_t *)&huemail);
}

void zllctrl_process_console_command( char *cmdBuff )
{
  // char cmdBuff[MAX_CONSOLE_CMD_LEN];
  uint16_t nwkAddr;
  uint8_t addrMode;
  uint8_t endpoint;
  uint8_t value;
  uint16_t transitionTime;
  
  getConsoleCommandParams(cmdBuff, &nwkAddr, &addrMode, &endpoint, &value, &transitionTime);   

  if((strstr(cmdBuff, "ping")) != 0)
  {
  	zllSocSysPing(NULL);
	uprintf_default("ping command executed\n\n");
  }
  else if((strstr(cmdBuff, "touchlink")) != 0)
  {      
    zllSocTouchLink(NULL);
    uprintf_default("touchlink command executed\n\n");
  }
  else if((strstr(cmdBuff, "sendresettofn")) != 0)
  {
    //sending of reset to fn must happen within a touchlink
    zllSocTouchLink(NULL);
    uprintf_default("press a key when device identyfies (ignored)\n");
    //getc(stdin);
    zllSocSendResetToFn(NULL);
  }  
  else if((strstr(cmdBuff, "resettofn")) != 0)
  {      
    zllSocResetToFn(NULL);
    uprintf_default("resettofn command executed\n\n");
  }
  else if((strstr(cmdBuff, "open")) != 0)
  {      
    zllSocOpenNwk(NULL, 0xFF);
    uprintf_default("zllSocOpenNwk command executed\n\n");
  }         
  else if((strstr(cmdBuff, "setstate")) != 0)
  {          
    zllSocSetState(NULL, value, nwkAddr, endpoint, addrMode);          
    uprintf_default("setstate command executed with params: \n");
    uprintf_default("    Network Addr    :0x%04x\n    End Point       :0x%02x\n    Addr Mode       :0x%02x\n    Value           :0x%02x\n\n", 
      nwkAddr, endpoint, addrMode, value);    
  }     
  else if((strstr(cmdBuff, "setlevel")) != 0)
  {      
    zllSocSetLevel(NULL, value, transitionTime, nwkAddr, endpoint, addrMode);   
    uprintf_default("setlevel command executed with params: \n");
    uprintf_default("    Network Addr    :0x%04x\n    End Point       :0x%02x\n    Addr Mode       :0x%02x\n    Value           :0x%02x\n    Transition Time :0x%04x\n\n", 
      nwkAddr, endpoint, addrMode, value, transitionTime);   
  }  
  else if((strstr(cmdBuff, "sethue")) != 0)
  {    
    zllSocSetHue(NULL, value, transitionTime, nwkAddr, endpoint, addrMode); 
    uprintf_default("sethue command executed with params: \n");
    uprintf_default("    Network Addr    :0x%04x\n    End Point       :0x%02x\n    Addr Mode       :0x%02x\n    Value           :0x%02x\n    Transition Time :0x%04x\n\n", 
      nwkAddr, endpoint, addrMode, value, transitionTime);
  } 
  else if((strstr(cmdBuff, "setsat")) != 0)
  {    
    zllSocSetSat(NULL, value, transitionTime, nwkAddr, endpoint, addrMode);    
    uprintf_default("setsat command executed with params: \n");
    uprintf_default("    Network Addr    :0x%04x\n    End Point       :0x%02x\n    Addr Mode       :0x%02x\n    Value           :0x%02x\n    Transition Time :0x%04x\n\n", 
      nwkAddr, endpoint, addrMode, value, transitionTime);
  }   
  else if((strstr(cmdBuff, "getstate")) != 0)
  {    
    zllSocGetState(NULL, nwkAddr, endpoint, addrMode);
    uprintf_default("getstate command executed wtih params: \n");
    uprintf_default("    Network Addr    :0x%04x\n    End Point       :0x%02x\n    Addr Mode       :0x%02x\n\n", 
      nwkAddr, endpoint, addrMode);
  }     
  else if((strstr(cmdBuff, "getlevel")) != 0)
  {      
    zllSocGetLevel(NULL, nwkAddr, endpoint, addrMode);    
    uprintf_default("getlevel command executed with params: \n");
    uprintf_default("    Network Addr    :0x%04x\n    End Point       :0x%02x\n    Addr Mode       :0x%02x\n\n", 
      nwkAddr, endpoint, addrMode);
  }  
  else if((strstr(cmdBuff, "gethue")) != 0)
  {    
    zllSocGetHue(NULL, nwkAddr, endpoint, addrMode);   
    uprintf_default("gethue command executed with params: \n");
    uprintf_default("    Network Addr    :0x%04x\n    End Point       :0x%02x\n    Addr Mode       :0x%02x\n\n", 
      nwkAddr, endpoint, addrMode);
  } 
  else if((strstr(cmdBuff, "getsat")) != 0)
  {    
    zllSocGetSat(NULL, nwkAddr, endpoint, addrMode);  
    uprintf_default("getsat command executed with params: \n");
    uprintf_default("    Network Addr    :0x%04x\n    End Point       :0x%02x\n    Addr Mode       :0x%02x\n\n", 
      nwkAddr, endpoint, addrMode);
  }
  else
  {
    uprintf_default("invalid command\n\n");
    commandUsage();
  }    
}

void getConsoleCommandParams(char* cmdBuff, uint16_t *nwkAddr, uint8_t *addrMode, uint8_t *ep, uint8_t *value, uint16_t *transitionTime)
{ 
  //set some default values
  uint32_t tmpInt;     
  
  if( getParam( cmdBuff, "-n", &tmpInt) )
  {
    savedNwkAddr = (uint16_t) tmpInt;
  }
  if( getParam( cmdBuff, "-m", &tmpInt) )
  {
    savedAddrMode = (uint8_t) tmpInt;
  }
  if( getParam( cmdBuff, "-e", &tmpInt) )
  {
    savedEp = (uint8_t) tmpInt;
  }
  if( getParam( cmdBuff, "-v", &tmpInt) )
  {
    savedValue = (uint8_t) tmpInt;
  }
  if( getParam( cmdBuff, "-t", &tmpInt) )
  {
    savedTransitionTime = (uint16_t) tmpInt;
  }        
        
  *nwkAddr = savedNwkAddr;    
  *addrMode = savedAddrMode;    
  *ep = savedEp;    
  *value = savedValue;    
  *transitionTime = savedTransitionTime;   

  return;         
}

uint32_t getParam( char *cmdBuff, char *paramId, uint32_t *paramInt)
{
  char* paramStrStart;
  char* paramStrEnd;
  //0x1234+null termination
  char paramStr[7];    
  uint32_t rtn = 0;
  
  memset(paramStr, 0, sizeof(paramStr));  
  paramStrStart = strstr(cmdBuff, paramId);
  
  if( paramStrStart )
  {
    //index past the param idenentifier "-?"
    paramStrStart+=2;
    //find the the end of the param text
    paramStrEnd = strstr(paramStrStart, " ");
    if( paramStrEnd )
    {
      if(paramStrEnd-paramStrStart > (sizeof(paramStr)-1))
      {
        //we are not on the last param, but the param str is too long
        strncpy( paramStr, paramStrStart, (sizeof(paramStr)-1));
        paramStr[sizeof(paramStr)-1] = '\0';  
      }
      else
      {
        //we are not on the last param so use the " " as the delimiter
        strncpy( paramStr, paramStrStart, paramStrEnd-paramStrStart);
        paramStr[paramStrEnd-paramStrStart] = '\0'; 
      }
    }
    
    else
    {
      //we are on the last param so use the just go the the end of the string 
      //(which will be null terminate). But make sure that it is not bigger
      //than our paramStr buffer. 
      if(strlen(paramStrStart) > (sizeof(paramStr)-1))
      {
        //command was entered wrong and paramStrStart will over flow the 
        //paramStr buffer.
        strncpy( paramStr, paramStrStart, (sizeof(paramStr)-1));
        paramStr[sizeof(paramStr)-1] = '\0';
      }
      else
      {
        //Param is the correct size so just copy it.
        strcpy( paramStr, paramStrStart);
        paramStr[strlen(paramStrStart)-1] = '\0';
      }
    }
    
    //was the param in hex or dec?
    if(strstr(paramStr, "0x"))   
    {
      //convert the hex value to an int.
      sscanf(paramStr, "0x%x", paramInt);
    }
    else
    {
      //assume that it ust be dec and convert to int.
      sscanf(paramStr, "%d", paramInt);
    }         
    
    //paramInt was set
    rtn = 1;
         
  }  
    
  return rtn;
}

hue_t *zllctrl_get_hue()
{
	return &g_hue;
}

uint8_t tlIndicationCb(epInfo_t *epInfo)
{
  uprintf_default("\ntlIndicationCb:\n    Network Addr : 0x%04x\n    End Point    : 0x%02x\n    Profile ID   : 0x%04x\n",
    epInfo->nwkAddr, epInfo->endpoint, epInfo->profileID);
  uprintf_default("    Device ID    : 0x%04x\n    Version      : 0x%02x\n    Status       : 0x%02x\n\n", 
     epInfo->deviceID, epInfo->version, epInfo->status); 
     
 //control this device by default
 savedNwkAddr =  epInfo->nwkAddr; 
 savedEp = epInfo->endpoint;

  /* TODO: add the new device to light list... */
  /* TODO: get state of new light */
  
  return 0;
}

uint8_t newDevIndicationCb(epInfo_t *epInfo)
{
  uprintf_default("\ntlIndicationCb:\n    Network Addr : 0x%04x\n    End Point    : 0x%02x\n    Profile ID   : 0x%04x\n",
    epInfo->nwkAddr, epInfo->endpoint, epInfo->profileID);
  uprintf_default("    Device ID    : 0x%04x\n    Version      : 0x%02x\n    Status       : 0x%02x\n", 
     epInfo->deviceID, epInfo->version, epInfo->status);
  uprintf_default("    IEEE Addr    : 0x%02x:0x%02x:0x%02x:0x%02x:0x%02x:0x%02x:0x%02x:0x%02x:\n\n\n", 
     epInfo->IEEEAddr[0], epInfo->IEEEAddr[1], epInfo->IEEEAddr[2], epInfo->IEEEAddr[3], 
     epInfo->IEEEAddr[4], epInfo->IEEEAddr[5], epInfo->IEEEAddr[6], epInfo->IEEEAddr[7]);      
     
 //control this device by default
 savedNwkAddr =  epInfo->nwkAddr; 
 savedEp = epInfo->endpoint;
   
  return 0;
}

uint32_t zllctrl_process_json_set_light(hue_t *hue, hue_mail_t *huemail)
{
    uint32_t ret = 0;
    uint16_t bitmap = huemail->set_one_light.flags;
    uint16_t dstAddr;
    uint16_t time;
    uint8_t endpoint;
    uint8_t addrMode = 0; // unicast
    hue_light_t *light, *newlight;
    uint8_t cmdbuf[64];

    if(huemail->set_one_light.light_id >= gNumHueLight)
    {
        uprintf(UPRINT_WARNING, UPRINT_BLK_HUE, "unknown light: %d\n", huemail->set_one_light.light_id);
        return -1;
    }

    light = &gHueLight[huemail->set_one_light.light_id];
    newlight = huemail->set_one_light.light;
    
    // update transition time first
    if(bitmap & (1<<HUE_LIGHT_STATE_TIME))
    {
        light->transitiontime = newlight->transitiontime;
    }

    if(light->reachable == 0)
        return -1;

    time = light->transitiontime;
    dstAddr = light->zig_addr.network_addr;
    endpoint = light->zig_addr.endpoint;

    if(bitmap & (1<<HUE_LIGHT_STATE_ON))
    {
        zllSocSetState(cmdbuf, newlight->on, dstAddr, endpoint, addrMode);
    }

    if(bitmap & (1<<HUE_LIGHT_STATE_BRI))
    {
        zllSocSetLevel(cmdbuf, newlight->bri, time, dstAddr, endpoint, addrMode);
    }

    if(bitmap & (1<<HUE_LIGHT_STATE_HUE))
    {
        // TI hue is 8 bits but philips is 16 bits
        zllSocSetHue(cmdbuf, newlight->hue, time, dstAddr, endpoint, addrMode);
    }

    if(bitmap & (1<<HUE_LIGHT_STATE_SAT))
    {
        zllSocSetSat(cmdbuf, newlight->sat, time, dstAddr, endpoint, addrMode);
    }
    
    if(bitmap & (1<<HUE_LIGHT_STATE_XY))
    {
        zllSocSetColor(cmdbuf, newlight->x, newlight->y, time, dstAddr, endpoint, addrMode);
    }

    if(bitmap & (1<<HUE_LIGHT_STATE_CT))
    {
        uprintf(UPRINT_WARNING, UPRINT_BLK_HUE, "unsupported hue state: ct\n");
    }

    if(bitmap & (1<<HUE_LIGHT_STATE_ALERT))
    {
        uprintf(UPRINT_WARNING, UPRINT_BLK_HUE, "unsupported hue state: alert\n");
    }

    if(bitmap & (1<<HUE_LIGHT_STATE_EFFECT))
    {
        uprintf(UPRINT_WARNING, UPRINT_BLK_HUE, "unsupported hue state: effect\n");
    }

    if(bitmap & (1<<HUE_LIGHT_STATE_BRIINC))
    {
        zllSocSetLevel(cmdbuf, light->bri + newlight->bri, time, dstAddr, endpoint, addrMode);
    }

    if(bitmap & (1<<HUE_LIGHT_STATE_HUEINC))
    {
        zllSocSetHue(cmdbuf, light->hue + newlight->hue, time, dstAddr, endpoint, addrMode);
    }

    if(bitmap & (1<<HUE_LIGHT_STATE_SATINC))
    {
        zllSocSetSat(cmdbuf, light->sat + newlight->sat, time, dstAddr, endpoint, addrMode);
    }

    if(bitmap & (1<<HUE_LIGHT_STATE_XYINC))
    {
        zllSocSetColor(cmdbuf, light->x+newlight->x, light->y+newlight->y, time, dstAddr, endpoint, addrMode);
    }

    if(bitmap & (1<<HUE_LIGHT_STATE_CTINC))
    {
        uprintf(UPRINT_WARNING, UPRINT_BLK_HUE, "unsupported hue state: ctinc\n");
    }

    return ret;
}

uint32_t zllctrl_process_json_search_lights(hue_t *hue, hue_mail_t *huemail)
{
    uint32_t ret = 0;
    uint8_t cmdbuf[64];
    
    /* issue a touch link command?? */
    zllSocTouchLink(cmdbuf);

    return ret;
}

uint32_t zllctrl_process_json_message(hue_t *hue, hue_mail_t *huemail)
{
    uint32_t ret = 0;

    switch(huemail->common.cmd)
    {
        case HUE_MAIL_CMD_SET_ONE_LIGHT:
            ret = zllctrl_process_json_set_light(hue, huemail);
            break;

        case HUE_MAIL_CMD_SEACH_NEW_LIGHTS:
            ret = zllctrl_process_json_search_lights(hue, huemail);
            break;

        default:
            ret = -1;
            uprintf(UPRINT_WARNING, UPRINT_BLK_HUE, "unsupported hue command: %d\n", huemail->common.cmd);
            break;
    }

    if(huemail->common.data)
        free(huemail->common.data);

    return ret;
}

static uint32_t zllctrl_mainloop(hue_t *hue)
{
    hue_mail_t huemail;
    zll_mail_t zllmail;
    int ret;
    uint32_t t1, t2;

    t1 = tick();
    /* after network is established, begin event loop */
    while(1)
    {
        /* process timers */
        t2 = tick();
        if(t2 > t1)
        {
            ptimer_consume_time(&g_zll_timer_table, t2-t1);
            t1 = t2;
        }

        /* wait for hue or console message to processing */
        ret = mbox_pend(&g_hue_mbox, (uint32_t *)&huemail, 0);
        if(ret == ROK)
        {
            if(huemail.common.cmd & HUE_MAIL_CMD_TYPE_MASK)
            {
                /* process requests from console */
                zllctrl_process_console_command((char *)huemail.console.data);
				free(huemail.console.data);
            }
            else
            {
                /* process requests from hue */
                zllctrl_process_json_message(hue, &huemail);
            }
        }

        /* wait for response and indication from soc */
        ret = mbox_pend(&g_zll_mbox, (uint32_t *)&zllmail, 2);
        if(ret == ROK)
        {
            /* process messages from zigbee device */
            zllctrl_process_soc_message(hue, zllmail.data, zllmail.length);
        }
    }

    return 0;
}

hue_light_t *zllctrl_find_light(uint16_t nwkAddr, uint8_t endpoint)
{
    int i;
    hue_light_t *light = NULL;

    for(i=0; i<gNumHueLight; i++)
    {
        if(gHueLight[i].zig_addr.network_addr == nwkAddr && gHueLight[i].zig_addr.endpoint == endpoint)
        {
            light = &gHueLight[i];
            break;
        }
    }

    return light;
}

int zllctrl_on_get_light_sat(uint16_t nwkAddr, uint8_t endpoint, uint8_t sat)
{
    hue_light_t *light;

    light = (hue_light_t *)zllctrl_find_light(nwkAddr, endpoint);
    if(light)
    {
         uprintf(UPRINT_DEBUG, UPRINT_BLK_HUE, "Network Addr: 0x%04x End Point: 0x%02x Saturation: %02x\n", 
                nwkAddr, endpoint, sat); 
        light->sat = sat;
    }

    return 0;
}


int zllctrl_on_get_light_hue(uint16_t nwkAddr, uint8_t endpoint, uint8_t hue)
{
    hue_light_t *light;

    light = (hue_light_t *)zllctrl_find_light(nwkAddr, endpoint);
    if(light)
    {
        uprintf(UPRINT_DEBUG, UPRINT_BLK_HUE, "Network Addr: 0x%04x End Point: 0x%02x Hue: %02x\n", 
                nwkAddr, endpoint, hue); 
        light->hue = hue;
    }

    return 0;
}


int zllctrl_on_get_light_level(uint16_t nwkAddr, uint8_t endpoint, uint8_t level)
{
    hue_light_t *light;

    light = (hue_light_t *)zllctrl_find_light(nwkAddr, endpoint);
    if(light)
    {
        uprintf(UPRINT_DEBUG, UPRINT_BLK_HUE, "Network Addr: 0x%04x End Point: 0x%02x brightless: %02x\n", 
                nwkAddr, endpoint, level); 
        light->bri = level;
    }

    return 0;
}


int zllctrl_on_get_light_on_off(uint16_t nwkAddr, uint8_t endpoint, uint8_t state)
{
    hue_light_t *light;

    light = (hue_light_t *)zllctrl_find_light(nwkAddr, endpoint);
    if(light)
    {
        uprintf(UPRINT_DEBUG, UPRINT_BLK_HUE, "Network Addr: 0x%04x End Point: 0x%02x on: %02x\n", 
                nwkAddr, endpoint, state); 
        light->on = state;
    }

    return 0;
}

/* interface provided to low level for indicating a new zll message */
int zllctrl_post_zll_message(uint8_t *message, uint16_t length)
{
    uint8_t *rpcmsg = NULL;
    int ret = 0;
    zll_mail_t zllmail;

    rpcmsg = malloc(length);
    if(rpcmsg)
    {
        memcpy(rpcmsg, message, length);

        zllmail.event = 0;
        zllmail.flags = 0;
        zllmail.data = rpcmsg;
        zllmail.length = length;

        ret = mbox_post(&g_zll_mbox, (uint32_t *)&zllmail);
    }
    else
        ret = -1;
    
    return ret;
}

int zllctrl_process_soc_message(hue_t *hue, uint8_t *data, uint16_t length)
{
    zllSocProcessRpc(data, length);
    if(data)
        free(data);

    return 0;
}

uint32_t zllctrl_start_soc_eventloop(uint32_t deadline, hue_t *hue, uint32_t (*endfunc)())
{
    int retCode, endCode = RTIMEOUT;
    int interval;
	zll_mail_t zllmail;
    uint32_t t1, t2;

    t1 = tick();

    interval = (deadline>0)?1:0;

    do{
        /* process messages from mbox */
    	if(mbox_pend(&g_zll_mbox, (uint32_t *)&zllmail, interval) == ROK)
    	{
    		retCode = zllctrl_process_soc_message(hue, zllmail.data, zllmail.length);
            if(endfunc)
            {
                endCode = endfunc(hue, zllmail.data, zllmail.length, retCode);
                if(endCode != RTIMEOUT)
                    break;
            }
    	}

        /* process timers */
        t2 = tick();
        if(t2 > t1)
        {
            ptimer_consume_time(&g_zll_timer_table, t2-t1);
            t1 = t2;
        }
    }while(t2 <= deadline);
    
	return endCode;
}

static uint32_t zllsoc_is_sysversion_processed(hue_t *hue, uint8_t *data, uint32_t length)
{
	if(hue->product_id != 0)
		return ROK;
	return RTIMEOUT;
}

static uint32_t zllsoc_is_devinfo_processed(hue_t *hue, uint8_t *data, uint32_t length)
{
	return RTIMEOUT;
}

/* establish the connection to soc */
uint32_t zllctrl_connect_to_soc(hue_t *hue)
{
	uint32_t ret;
	
	/* get sys version */
	zllSocSysVersion(NULL);
    ret = zllctrl_start_soc_eventloop(ZLL_RESP_DEFAULT_TIMEOUT+tick(), hue, zllsoc_is_sysversion_processed);
    if(ret == RTIMEOUT)
    {
        uprintf(UPRINT_ERROR, UPRINT_BLK_HUE, "can't get sysversion: %d\n", ret);
        return ret;
    }

	/* get device info */
	zllSocUtilGetDevInfo(NULL);
    ret = zllctrl_start_soc_eventloop(ZLL_RESP_DEFAULT_TIMEOUT+tick(), hue, zllsoc_is_devinfo_processed);
    if(ret == RTIMEOUT)
    {
        uprintf(UPRINT_ERROR, UPRINT_BLK_HUE, "can't get device info\n");
        return ret;
    }

    return ret;
}

uint32_t zllctrl_network_setup(hue_t *hue)
{
	uint32_t ret;
    

    /* touch link: encapsulated in a app message */
    zllSocTouchLink(NULL);
    ret = zllctrl_start_soc_eventloop(ZLL_RESP_DEFAULT_TIMEOUT+tick(), hue, NULL);
    if(ret != ROK)
    {
        uprintf(UPRINT_ERROR, UPRINT_BLK_HUE, "can't touch link: %d\n", ret);
        return ret;
    }

    /* permit join forever */
    zllSocOpenNwk(NULL, 0xFF);
    ret = zllctrl_start_soc_eventloop(ZLL_RESP_DEFAULT_TIMEOUT+tick(), hue, NULL);
    if(ret != ROK)
    {
        uprintf(UPRINT_ERROR, UPRINT_BLK_HUE, "can't permit join: %d\n", ret);
        return ret;
    }

    return ret;
}

void zllctrl_set_state(hue_t *hue, uint32_t state)
{
    uprintf(UPRINT_INFO, UPRINT_BLK_HUE, "hue state 0x%x --> 0x%x\n", hue->state, state);
    hue->state = state;
}

static void zllctrl_task(void *p)
{
	hue_t *hue;
	uint32_t ret;

	hue = (hue_t *)p;
	zllctrl_set_state(hue, HUE_STATE_INIT);
	
    /* create timer table */
    ret = ptimer_init(&g_zll_timer_table, 16 /* nSlot */);
    assert(ret == 0);

	/* connect to Soc */
	ret = zllctrl_connect_to_soc(hue);
	if(ret != 0)
	{
		uprintf(UPRINT_ERROR, UPRINT_BLK_HUE, "failed to connect to cc2530 Soc: ret=%d\n", ret);
		return;
	}

	zllctrl_set_state(hue, HUE_STATE_CONNECTED);
	
	/* setup network */
    ret = zllctrl_network_setup(hue);
    if(ret != 0)
	{
		uprintf(UPRINT_ERROR, UPRINT_BLK_HUE, "failed to establish network: ret=%d\n", ret);
		return;
	}

    zllctrl_set_state(hue, HUE_STATE_NETSETUP);

	zllctrl_mainloop(hue);
}


int zllctrl_init()
{
	/* create a zllhue task */
	task_t t;

	//set some default values for console
	savedNwkAddr = 0x0002;
	savedAddrMode = 0x02;	 
	savedEp = 0x0b;    
	savedValue = 0x0;	 
	savedTransitionTime = 0x1;

	mbox_initialize(&g_hue_mbox, sizeof(hue_mail_t)/sizeof(int), HUE_MBOX_SIZE, hue_mbox_data);
    mbox_initialize(&g_zll_mbox, sizeof(zll_mail_t)/sizeof(int), ZLL_MBOX_SIZE, zll_mbox_data);

	t = task_create("tzll", zllctrl_task, &g_hue, zll_task_stack, TZLL_STACK_SIZE, TZLL_PRIORITY, 20, 0);
	assert(t != RERROR);
	task_resume_noschedule(t);
	return 0;
}

