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
#include "journal.h"
#include "hue.h"
#include "zll_controller.h"
#include "zllSocCmd.h"
#include "zcl_ll.h"
#include "stm32f2xx.h"
#include "stm32f2xx_hal.h"

mbox_t g_hue_mbox;  // Rx message from HTTP or console

hue_t g_hue;
ptimer_table_t g_zll_timer_table;


static hue_mail_t hue_mbox_data[HUE_MBOX_SIZE];
static char zll_task_stack[TZLL_STACK_SIZE];

#define MAX_CONSOLE_CMD_LEN 128

void commandUsage( void );
void zllctrl_process_console_command( char *cmdBuff );
void getConsoleCommandParams(char* cmdBuff, uint16_t *nwkAddr, uint8_t *addrMode, uint8_t *ep, uint16_t *value, uint16_t *transitionTime);
uint32_t getParam( char *cmdBuff, char *paramId, uint32_t *paramInt);

extern void ssdp_start_statemachine_unicast(uint32_t destIp, uint32_t destPort, uint16_t interval);
extern int ssdp_init();

static uint16_t savedNwkAddr;    
static uint8_t savedAddrMode;    
static uint8_t savedEp;    
static uint16_t savedValue;    
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
    uprintf_default("getsat\n");
	uprintf_default("sendraw\n\n");
    
    uprintf_default("Parameters:\n");
    uprintf_default("-n<network address>\n");    
    uprintf_default("-e<end point>\n");
    uprintf_default("-m<address mode 1=groupcast, 2=unicast>\n");
    uprintf_default("-v<value>\n");
    uprintf_default("-t<transition time in 10ms>\n\n");
    
	uprintf_default("examples: \n");  
    uprintf_default("turn on a new light (0x0 is white and 0xFE is the fully saturated color): setsat -v1\n");    
    uprintf_default("move to read hue over 3s: sethue -n0x0003 -e0xb -m0x2 -v0 -t30\n");
    uprintf_default("change to a blue hue: sethue -v0xAA \n\n");
}

int zllctrl_post_console_command(char *command)
{
    hue_mail_t huemail;

    huemail.console.cmd = HUE_MAIL_CMD_CONSOLE;
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
  uint16_t value;
  uint16_t transitionTime;
  
  getConsoleCommandParams(cmdBuff, &nwkAddr, &addrMode, &endpoint, &value, &transitionTime);   

  if((strstr(cmdBuff, "sendraw")) != 0)
  {
    zllSocSendRaw(cmdBuff);
	uprintf_default("sendraw command executed\n\n");
  }
  else if((strstr(cmdBuff, "ping")) != 0)
  {
  	zllSocSysPing(NULL);
	uprintf_default("ping command executed\n\n");
  }
  else if((strstr(cmdBuff, "txpower")) != 0)
  {
    zllSocSysSetTxPower(NULL, value);
    uprintf_default("set Tx Power: %d dBm\n\n", value);
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
  else if((strstr(cmdBuff, "setct")) != 0)
  {
    zllSocSetColorTemperature(NULL, value, transitionTime, nwkAddr, endpoint, addrMode);    
    uprintf_default("setct command executed with params: \n");
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

void getConsoleCommandParams(char* cmdBuff, uint16_t *nwkAddr, uint8_t *addrMode, uint8_t *ep, uint16_t *value, uint16_t *transitionTime)
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
    savedValue = (uint16_t) tmpInt;
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
      //sscanf(paramStr, "0x%x", paramInt);
      *paramInt = strtol(paramStr, NULL, 16);
    }
    else
    {
      //assume that it ust be dec and convert to int.
      //sscanf(paramStr, "%d", paramInt);
      *paramInt = strtol(paramStr, NULL, 10);
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

uint8_t zllctrl_process_touchlink_indication(epInfo_t *epInfo)
{
  hue_light_t *light;

  uprintf(UPRINT_INFO, UPRINT_BLK_HUE, "touchlink ind: NetworkAddr=0x%04x EndPoint=0x%02x ProfileID=0x%04x DeviceID=0x%04x Version=0x%02x Status=0x%02x\n",
    epInfo->nwkAddr, epInfo->endpoint, epInfo->profileID, epInfo->deviceID, epInfo->version, epInfo->status);
     
  //control this device by default
  savedNwkAddr =  epInfo->nwkAddr; 
  savedEp = epInfo->endpoint;

  if(epInfo->profileID == ZLL_PROFILE_ID)
  {
    /* add the new device to light list... */
    light = zllctrl_find_light_by_shortaddr(epInfo->nwkAddr, epInfo->endpoint);
    if(light == NULL)
    {
      light = zllctrl_create_light(epInfo);
    
      /* TODO: get state of new light */
    }

    if(light != NULL)
    {
      light->reachable = 1;
    }
  }
  
  return 0;
}

uint8_t zllctrl_process_newdev_indication(epInfo_t *epInfo)
{
  hue_light_t *light;

  uprintf(UPRINT_INFO, UPRINT_BLK_HUE, "newDevIndicationCb: NetworkAddr=0x%04x EndPoint=0x%02x ProfileID=0x%04x DeviceID=0x%04x Version=0x%02x Status=0x%02x\n",
		epInfo->nwkAddr, epInfo->endpoint, epInfo->profileID, epInfo->deviceID, epInfo->version, epInfo->status);
  uprintf(UPRINT_INFO, UPRINT_BLK_HUE, "    IEEE Addr: 0x%02x:0x%02x:0x%02x:0x%02x:0x%02x:0x%02x:0x%02x:0x%02x:\n", 
     epInfo->IEEEAddr[0], epInfo->IEEEAddr[1], epInfo->IEEEAddr[2], epInfo->IEEEAddr[3], 
     epInfo->IEEEAddr[4], epInfo->IEEEAddr[5], epInfo->IEEEAddr[6], epInfo->IEEEAddr[7]);      
     
  //control this device by default
  savedNwkAddr =  epInfo->nwkAddr; 
  savedEp = epInfo->endpoint;
   
  if(epInfo->profileID == ZLL_PROFILE_ID)
  {
    /* add the new device to light list... */
    light = zllctrl_find_light_by_shortaddr(epInfo->nwkAddr, epInfo->endpoint);
    if(light == NULL)
    {
      light = zllctrl_create_light(epInfo);

      /* TODO: get state of new light */
    }

    if(light != NULL)
    {
      light->reachable = 1;
    }
  }
  
  return 0;
}

#define CC2530_PROGRAMMING_INTERVAL 100 // prevent programming cc2530 too fast, in us
extern void udelay(uint32_t us);
uint32_t zllctrl_process_json_set_light(hue_t *hue, hue_mail_t *huemail)
{
    uint32_t ret = 0;
    uint16_t bitmap = huemail->set_one_light.flags;
    uint16_t dstAddr;
    uint16_t time;
    uint8_t endpoint;
    uint8_t addrMode = 2; //short address // unicast
    hue_light_t *light, *newlight;
    uint8_t cmdbuf[64];

    light = hue_find_light_by_id(huemail->set_one_light.light_id);
    if(light == NULL)
    {
        uprintf(UPRINT_WARNING, UPRINT_BLK_HUE, "unknown light: %d\n", huemail->set_one_light.light_id);
        return -1;
    }

    newlight = huemail->set_one_light.light;
    assert(newlight);
        
    // update transition time first
    if(bitmap & (1<<HUE_STATE_BIT_TRANSTIME))
    {
        light->transitiontime = newlight->transitiontime;
    }

    if(light->reachable == 0)
    {
        uprintf(UPRINT_WARNING, UPRINT_BLK_HUE, "light is unreachable: %d\n", huemail->set_one_light.light_id);
        return -1;
    }

    time = light->transitiontime;
    dstAddr = light->ep_info.nwkAddr;
    endpoint = light->ep_info.endpoint;

    uprintf(UPRINT_INFO, UPRINT_BLK_HUE, "set light, dstAddr=0x%x endpoint=0x%x bipmap=0x%x\n", 
            dstAddr, endpoint, bitmap);

    if(bitmap & (1<<HUE_STATE_BIT_ON))
    {
        zllSocSetState(cmdbuf, newlight->on, dstAddr, endpoint, addrMode);
        light->on = newlight->on;
		udelay(CC2530_PROGRAMMING_INTERVAL);  
    }

    if(bitmap & (1<<HUE_STATE_BIT_BRI))
    {
        zllSocSetLevel(cmdbuf, newlight->bri, time, dstAddr, endpoint, addrMode);
        light->bri = newlight->bri;
		udelay(CC2530_PROGRAMMING_INTERVAL); 
    }

    if(bitmap & (1<<HUE_STATE_BIT_HUE))
    {
        // TI hue is 8 bits but philips is 16 bits
        zllSocSetHue(cmdbuf, newlight->hue, time, dstAddr, endpoint, addrMode);
        light->hue = newlight->hue;
		udelay(CC2530_PROGRAMMING_INTERVAL); 
    }

    if(bitmap & (1<<HUE_STATE_BIT_SAT))
    {
        zllSocSetSat(cmdbuf, newlight->sat, time, dstAddr, endpoint, addrMode);
        light->sat = newlight->sat;
		udelay(CC2530_PROGRAMMING_INTERVAL); 
    }
    
    if(bitmap & (1<<HUE_STATE_BIT_XY))
    {
        zllSocSetColor(cmdbuf, newlight->x, newlight->y, time, dstAddr, endpoint, addrMode);
		udelay(CC2530_PROGRAMMING_INTERVAL); 
    }

    if(bitmap & (1<<HUE_STATE_BIT_CT))
    {
        uprintf(UPRINT_INFO, UPRINT_BLK_HUE, "set ct: 0x%x\n", newlight->ct);
        zllSocSetColorTemperature(cmdbuf, newlight->ct, time, dstAddr, endpoint, addrMode);
		udelay(CC2530_PROGRAMMING_INTERVAL); 
    }

    if(bitmap & (1<<HUE_STATE_BIT_ALERT))
    {
        uprintf(UPRINT_WARNING, UPRINT_BLK_HUE, "unsupported hue state: alert\n");
    }

    if(bitmap & (1<<HUE_STATE_BIT_EFFECT))
    {
        uprintf(UPRINT_WARNING, UPRINT_BLK_HUE, "unsupported hue state: effect\n");
    }

    if(bitmap & (1<<HUE_STATE_BIT_BRIINC))
    {
        zllSocSetLevel(cmdbuf, light->bri + newlight->bri, time, dstAddr, endpoint, addrMode);
        light->bri += newlight->bri;
		udelay(CC2530_PROGRAMMING_INTERVAL); 
    }

    if(bitmap & (1<<HUE_STATE_BIT_HUEINC))
    {
        zllSocSetHue(cmdbuf, light->hue + newlight->hue, time, dstAddr, endpoint, addrMode);
        light->hue += newlight->hue;
		udelay(CC2530_PROGRAMMING_INTERVAL); 
    }

    if(bitmap & (1<<HUE_STATE_BIT_SATINC))
    {
        zllSocSetSat(cmdbuf, light->sat + newlight->sat, time, dstAddr, endpoint, addrMode);
        light->sat += newlight->sat;
		udelay(CC2530_PROGRAMMING_INTERVAL); 
    }

    if(bitmap & (1<<HUE_STATE_BIT_XYINC))
    {
        zllSocSetColor(cmdbuf, light->x+newlight->x, light->y+newlight->y, time, dstAddr, endpoint, addrMode);
        // xyinc TBD
        udelay(CC2530_PROGRAMMING_INTERVAL); 
    }

    if(bitmap & (1<<HUE_STATE_BIT_CTINC))
    {
        uprintf(UPRINT_WARNING, UPRINT_BLK_HUE, "unsupported hue state: ctinc\n");
    }

    return ret;
}

uint32_t zllctrl_process_json_search_lights(hue_t *hue, hue_mail_t *huemail)
{
    uint32_t ret = 0;
    //uint8_t cmdbuf[64];
    
    /* issue a touch link command */
    uprintf(UPRINT_INFO, UPRINT_BLK_HUE, "touchlink ...\n");
    zllSocTouchLink(NULL);

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

/* GPIO init for LEDs 
 * PE0: LED_ETH
 * PE1: KEY_IN
 * PE2: LED_PORTAL
 * PE3: LED_ZIGBEE
 * PE4: LED_POWER
 */
static void zllctrl_gpio_init()
{
    GPIO_InitTypeDef GPIO_InitStructure;

	__HAL_RCC_GPIOE_CLK_ENABLE();
	
	GPIO_InitStructure.Pin = GPIO_PIN_0 | GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4;
	GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_MEDIUM;
	GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStructure.Pull = GPIO_NOPULL;
	GPIO_InitStructure.Alternate = 0;
	
	HAL_GPIO_Init(GPIOE, &GPIO_InitStructure);

	HAL_GPIO_WritePin(GPIOE, GPIO_PIN_4, GPIO_PIN_SET);  // power led
    HAL_GPIO_WritePin(GPIOE, GPIO_PIN_0, GPIO_PIN_SET); // ethernet Led

	GPIO_InitStructure.Pin = GPIO_PIN_1;
	GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_MEDIUM;
	GPIO_InitStructure.Mode = GPIO_MODE_INPUT;
	GPIO_InitStructure.Pull = GPIO_PULLUP;
	GPIO_InitStructure.Alternate = 0;
	HAL_GPIO_Init(GPIOE, &GPIO_InitStructure);
}

static ptimer_t zllctrl_timer[5];

static int32_t led_off(void *tim, uint32_t param0, uint32_t param1)
{
	uint32_t led_pin = ((ptimer_t *)tim)->param[0];
	HAL_GPIO_WritePin(GPIOE, 1<<led_pin, GPIO_PIN_RESET);
	return PTIMER_RET_OK;
}

// led is GPIO pin
static void zllctrl_blink_led(uint16_t led, uint32_t duration)
{
	ptimer_t *timer;

	timer = &zllctrl_timer[led&0x1F];
	timer->onexpired_func = led_off;
	timer->param[0] = led;  // pin of LED
	// set led on
	HAL_GPIO_WritePin(GPIOE, 1<<led, GPIO_PIN_SET);
	if(ptimer_is_running(timer))
		ptimer_cancel(&g_zll_timer_table, timer);
	ptimer_start(&g_zll_timer_table, timer, duration);
}

static int32_t button_monitor(void *tim, uint32_t param0, uint32_t param1)
{
    uint8_t status;
    uint32_t now = tick();
    ptimer_t *timer = (ptimer_t *)tim;

	status = HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_1);
    if((status == GPIO_PIN_RESET) && (now - param0 > 100))
    {
        // button active and 1s after last touchlink
        zllctrl_blink_led(2, 50); // set portal led
        uprintf(UPRINT_INFO, UPRINT_BLK_HUE, "touchlink ...\n");
        zllSocTouchLink(NULL);
        timer->param[0] = now;
    }

    return PTIMER_RET_OK;
}


static uint32_t zllctrl_mainloop(hue_t *hue)
{
    hue_mail_t huemail;
    int ret;
    uint32_t t1, t2;
    ptimer_t *buttontimer = &zllctrl_timer[1];

    /* init GPIO: PE0/2/3/4 is LED and PE1 is touchlink button */
    zllctrl_gpio_init();

    /* start a timer to monitor button event (via gpio) every 0.5s */
    buttontimer->flags = PTIMER_FLAG_PERIODIC;
    buttontimer->onexpired_func = button_monitor;
    buttontimer->param[0] = 0; // param[0] represent the last time of touchlink
    ptimer_start(&g_zll_timer_table, buttontimer, 10);

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
        ret = mbox_pend(&g_hue_mbox, (uint32_t *)&huemail, 100);
        if(ret == ROK)
        {
            if(huemail.common.cmd == HUE_MAIL_CMD_CONSOLE)
            {
                /* process requests from console */
                zllctrl_process_console_command((char *)huemail.console.data);
				free(huemail.console.data);
            }
            else if(huemail.common.cmd == HUE_MAIL_CMD_SSDP)
            {
                /* process SSDP discovery request: send back SSDP alive message */
                ssdp_start_statemachine_unicast(huemail.ssdp.ipaddr, huemail.ssdp.port, huemail.ssdp.interval);
            }
			else if(huemail.common.cmd == HUE_MAIL_CMD_SOCMSG)
			{
				journal_user_defined(JOURNAL_TYPE_CLASS1MAX, 1);
	            /* process messages from zigbee device */
				zllctrl_blink_led(3, 50); // zigbee status led
	            zllctrl_process_soc_message(hue, huemail.socmsg.data, huemail.socmsg.length);
			}
            else
            {
                /* process requests from hue */
                zllctrl_process_json_message(hue, &huemail);
            }
        }
    }

    return 0;
}

hue_light_t *zllctrl_find_light_by_shortaddr(uint16_t nwkAddr, uint8_t endpoint)
{
    int i;
    hue_light_t *light = NULL;

    for(i=0; i<gNumHueLight; i++)
    {
        if(gHueLight[i].ep_info.nwkAddr == nwkAddr && gHueLight[i].ep_info.endpoint == endpoint)
        {
            light = &gHueLight[i];
            break;
        }
    }

    return light;
}

hue_light_t *zllctrl_find_light_by_ieeeaddr(uint8_t *ieeeaddr)
{
    int i;
    hue_light_t *light = NULL;

    for(i=0; i<gNumHueLight; i++)
    {
        if(memcmp(gHueLight[i].ep_info.IEEEAddr, ieeeaddr, 8) == 0)
        {
            light = &gHueLight[i];
            break;
        }
    }

    return light;
}

hue_light_t *zllctrl_create_light(epInfo_t *epInfo)
{
    hue_light_t *light = NULL;

    if(gNumHueLight < HUE_MAX_LIGHTS)
    {
        light = &gHueLight[gNumHueLight];
        light->ep_info = *epInfo;
        gNumHueLight ++;
    }
    else
    {
        uprintf(UPRINT_WARNING, UPRINT_BLK_HUE, "create light failed: nwkAddr=0x%x endpoint=0x%x\n",
            epInfo->nwkAddr, epInfo->endpoint);
    }

    return light;
}

int zllctrl_on_get_light_sat(uint16_t nwkAddr, uint8_t endpoint, uint8_t sat)
{
    hue_light_t *light;

    light = (hue_light_t *)zllctrl_find_light_by_shortaddr(nwkAddr, endpoint);
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

    light = (hue_light_t *)zllctrl_find_light_by_shortaddr(nwkAddr, endpoint);
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

    light = (hue_light_t *)zllctrl_find_light_by_shortaddr(nwkAddr, endpoint);
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

    light = (hue_light_t *)zllctrl_find_light_by_shortaddr(nwkAddr, endpoint);
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
    hue_mail_t huemail;

    rpcmsg = malloc(length);
    if(rpcmsg)
    {
        memcpy(rpcmsg, message, length);

        huemail.socmsg.cmd = HUE_MAIL_CMD_SOCMSG;
        huemail.socmsg.filler1 = 0;
        huemail.socmsg.data = rpcmsg;
        huemail.socmsg.length = length;

        //kprintf("zllctrl_post_zll_message %d: 0x%02x 0x%02x\n", length, message[2], message[3]);

        ret = mbox_post(&g_hue_mbox, (uint32_t *)&huemail);
    }
    else
        ret = -1;
    
    return ret;
}

int zllctrl_process_soc_message(hue_t *hue, uint8_t *data, uint16_t length)
{
	/* copy soc message */
	memcpy(hue->socbuf, data, length);
	uprintf(UPRINT_DEBUG, UPRINT_BLK_HUE, "Rx soc MSG (%d): %08x %08x %08x %08x %08x\n", 
			length, *(uint32_t *)&data[0], *(uint32_t *)&data[4], 
			*(uint32_t *)&data[8], *(uint32_t *)&data[12], *(uint32_t *)&data[16]);
    zllSocProcessRpc(data, length);
    if(data)
        free(data);

    return 0;
}

uint32_t zllctrl_start_soc_eventloop(uint32_t deadline, hue_t *hue, uint32_t (*endfunc)())
{
    int retCode, endCode = RTIMEOUT;
    int interval;
	hue_mail_t huemail;
    uint32_t t1, t2;

    t1 = tick();

    interval = (deadline>0)?1:0;

    do{
        /* process messages from mbox */
    	if(mbox_pend(&g_hue_mbox, (uint32_t *)&huemail, interval) == ROK)
    	{
    		if(huemail.common.cmd == HUE_MAIL_CMD_SOCMSG)
    		{
	    		retCode = zllctrl_process_soc_message(hue, huemail.socmsg.data, huemail.socmsg.length);
	            if(endfunc)
	            {
	                endCode = endfunc(hue, huemail.socmsg.data, huemail.socmsg.length, retCode);
	                if(endCode != RTIMEOUT)
	                    break;
	            }
	            else
	                endCode = ROK;
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
	return ROK;
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

#if 0
	/* get device info */
	zllSocUtilGetDevInfo(NULL);
    ret = zllctrl_start_soc_eventloop(ZLL_RESP_DEFAULT_TIMEOUT+tick(), hue, zllsoc_is_devinfo_processed);
    if(ret == RTIMEOUT)
    {
        uprintf(UPRINT_ERROR, UPRINT_BLK_HUE, "can't get device info\n");
        return ret;
    }
#endif
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

	//set some default values for console
	savedNwkAddr = 0x0002;
	savedAddrMode = 0x02;	 
	savedEp = 0x0b;    
	savedValue = 0x0;	 
	savedTransitionTime = 0x1;

	mbox_initialize(&g_hue_mbox, sizeof(hue_mail_t)/sizeof(int), HUE_MBOX_SIZE, hue_mbox_data);
	IPC_SET_OWNER(&g_hue_mbox, TASK_T(current)|0x20);
	
	//uprintf_set_enable(UPRINT_INFO, UPRINT_BLK_HUE, 1);

	// delay 3s to wait other is up
	task_delay(3*100);
	uprintf(UPRINT_INFO, UPRINT_BLK_HUE, "zllctrl_task is active\n");
	
	hue = (hue_t *)p;
    hue_data_init(hue);
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

    ssdp_init();
   
	zllctrl_mainloop(hue);
}


int zllctrl_init()
{
	/* create a zllhue task */
	task_t t;

	t = task_create("tzll", zllctrl_task, &g_hue, zll_task_stack, TZLL_STACK_SIZE, TZLL_PRIORITY, 20, 0);
	assert(t != RERROR);
	task_resume_noschedule(t);
	return 0;
}



