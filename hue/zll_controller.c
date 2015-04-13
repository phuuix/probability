/*
 * zll_controller.c
 *
 * This module demonstrates how to use the API for the zll SoC 
 * Host Interface.
 *
 * Copyright (C) 2013 Texas Instruments Incorporated - http://www.ti.com/ 
 * 
 * 
 *  Redistribution and use in source and binary forms, with or without 
 *  modification, are permitted provided that the following conditions 
 *  are met:
 *
 *    Redistributions of source code must retain the above copyright 
 *    notice, this list of conditions and the following disclaimer.
 *
 *    Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the 
 *    documentation and/or other materials provided with the   
 *    distribution.
 *
 *    Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "zllSocCmd.h"
#include "uprintf.h"

#define MAX_CONSOLE_CMD_LEN 128

void commandUsage( void );
void processConsoleCommand( char *cmdBuff );
void getConsoleCommandParams(char* cmdBuff, uint16_t *nwkAddr, uint8_t *addrMode, uint8_t *ep, uint8_t *value, uint16_t *transitionTime);
uint32_t getParam( char *cmdBuff, char *paramId, uint32_t *paramInt);
uint8_t tlIndicationCb(epInfo_t *epInfo);
uint8_t newDevIndicationCb(epInfo_t *epInfo);
uint8_t zclGetStateCb(uint8_t state, uint16_t nwkAddr, uint8_t endpoint);
uint8_t zclGetLevelCb(uint8_t level, uint16_t nwkAddr, uint8_t endpoint);
uint8_t zclGetHueCb(uint8_t hue, uint16_t nwkAddr, uint8_t endpoint);
uint8_t zclGetSatCb(uint8_t sat, uint16_t nwkAddr, uint8_t endpoint);

static zllSocCallbacks_t zllSocCbs =
{
  tlIndicationCb,        // pfnTlIndicationCb - TouchLink Indication callback
  newDevIndicationCb,  // pfnNewDevIndicationCb - New Device Indication callback
  zclGetStateCb,         // pfnZclGetHueCb - ZCL response callback for get Hue
  zclGetLevelCb,         //pfnZclGetSatCb - ZCL response callback for get Sat
  zclGetHueCb,           //pfnZclGetLevelCb_t - ZCL response callback for get Level
  zclGetSatCb           //pfnZclGetStateCb - ZCL response callback for get State
};

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

int zllctrl_init()
{    
  zllSocRegisterCallbacks( zllSocCbs );
  
  //set some default values
  savedNwkAddr = 0x0002;    
  savedAddrMode = 0x02;    
  savedEp = 0x0b;    
  savedValue = 0x0;    
  savedTransitionTime = 0x1;

  return 0;
}

void processConsoleCommand( char *cmdBuff )
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
  	zllSocSysPing();
	uprintf_default("ping command executed\n\n");
  }
  else if((strstr(cmdBuff, "touchlink")) != 0)
  {      
    zllSocTouchLink();
    uprintf_default("touchlink command executed\n\n");
  }
  else if((strstr(cmdBuff, "sendresettofn")) != 0)
  {
    //sending of reset to fn must happen within a touchlink
    zllSocTouchLink();
    uprintf_default("press a key when device identyfies (ignored)\n");
    //getc(stdin);
    zllSocSendResetToFn();
  }  
  else if((strstr(cmdBuff, "resettofn")) != 0)
  {      
    zllSocResetToFn();
    uprintf_default("resettofn command executed\n\n");
  }
  else if((strstr(cmdBuff, "open")) != 0)
  {      
    zllSocOpenNwk(60);
    uprintf_default("zllSocOpenNwk command executed\n\n");
  }         
  else if((strstr(cmdBuff, "setstate")) != 0)
  {          
    zllSocSetState(value, nwkAddr, endpoint, addrMode);          
    uprintf_default("setstate command executed with params: \n");
    uprintf_default("    Network Addr    :0x%04x\n    End Point       :0x%02x\n    Addr Mode       :0x%02x\n    Value           :0x%02x\n\n", 
      nwkAddr, endpoint, addrMode, value);    
  }     
  else if((strstr(cmdBuff, "setlevel")) != 0)
  {      
    zllSocSetLevel(value, transitionTime, nwkAddr, endpoint, addrMode);   
    uprintf_default("setlevel command executed with params: \n");
    uprintf_default("    Network Addr    :0x%04x\n    End Point       :0x%02x\n    Addr Mode       :0x%02x\n    Value           :0x%02x\n    Transition Time :0x%04x\n\n", 
      nwkAddr, endpoint, addrMode, value, transitionTime);   
  }  
  else if((strstr(cmdBuff, "sethue")) != 0)
  {    
    zllSocSetHue(value, transitionTime, nwkAddr, endpoint, addrMode); 
    uprintf_default("sethue command executed with params: \n");
    uprintf_default("    Network Addr    :0x%04x\n    End Point       :0x%02x\n    Addr Mode       :0x%02x\n    Value           :0x%02x\n    Transition Time :0x%04x\n\n", 
      nwkAddr, endpoint, addrMode, value, transitionTime);
  } 
  else if((strstr(cmdBuff, "setsat")) != 0)
  {    
    zllSocSetSat(value, transitionTime, nwkAddr, endpoint, addrMode);    
    uprintf_default("setsat command executed with params: \n");
    uprintf_default("    Network Addr    :0x%04x\n    End Point       :0x%02x\n    Addr Mode       :0x%02x\n    Value           :0x%02x\n    Transition Time :0x%04x\n\n", 
      nwkAddr, endpoint, addrMode, value, transitionTime);
  }   
  else if((strstr(cmdBuff, "getstate")) != 0)
  {    
    zllSocGetState(nwkAddr, endpoint, addrMode);
    uprintf_default("getstate command executed wtih params: \n");
    uprintf_default("    Network Addr    :0x%04x\n    End Point       :0x%02x\n    Addr Mode       :0x%02x\n\n", 
      nwkAddr, endpoint, addrMode);
  }     
  else if((strstr(cmdBuff, "getlevel")) != 0)
  {      
    zllSocGetLevel(nwkAddr, endpoint, addrMode);    
    uprintf_default("getlevel command executed with params: \n");
    uprintf_default("    Network Addr    :0x%04x\n    End Point       :0x%02x\n    Addr Mode       :0x%02x\n\n", 
      nwkAddr, endpoint, addrMode);
  }  
  else if((strstr(cmdBuff, "gethue")) != 0)
  {    
    zllSocGetHue(nwkAddr, endpoint, addrMode);   
    uprintf_default("gethue command executed with params: \n");
    uprintf_default("    Network Addr    :0x%04x\n    End Point       :0x%02x\n    Addr Mode       :0x%02x\n\n", 
      nwkAddr, endpoint, addrMode);
  } 
  else if((strstr(cmdBuff, "getsat")) != 0)
  {    
    zllSocGetSat(nwkAddr, endpoint, addrMode);  
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
  
uint8_t tlIndicationCb(epInfo_t *epInfo)
{
  uprintf_default("\ntlIndicationCb:\n    Network Addr : 0x%04x\n    End Point    : 0x%02x\n    Profile ID   : 0x%04x\n",
    epInfo->nwkAddr, epInfo->endpoint, epInfo->profileID);
  uprintf_default("    Device ID    : 0x%04x\n    Version      : 0x%02x\n    Status       : 0x%02x\n\n", 
     epInfo->deviceID, epInfo->version, epInfo->status); 
     
 //control this device by default
 savedNwkAddr =  epInfo->nwkAddr; 
 savedEp = epInfo->endpoint;
   
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

uint8_t zclGetStateCb(uint8_t state, uint16_t nwkAddr, uint8_t endpoint)
{
  uprintf_default("\nzclGetStateCb:\n    Network Addr : 0x%04x\n    End Point    : 0x%02x\n    State        : 0x%02x\n\n", 
    nwkAddr, endpoint, state); 
  return 0;  
}

uint8_t zclGetLevelCb(uint8_t level, uint16_t nwkAddr, uint8_t endpoint)
{
  uprintf_default("\nzclGetLevelCb:\n    Network Addr : 0x%04x\n    End Point    : 0x%02x\n    Level        : %02x\n\n", 
    nwkAddr, endpoint, level); 
  return 0;  
}

uint8_t zclGetHueCb(uint8_t hue, uint16_t nwkAddr, uint8_t endpoint)
{
  uprintf_default("\nzclGetHueCb:\n    Network Addr : 0x%04x\n    End Point    : 0x%02x\n    Hue          : %02x\n\n", nwkAddr, endpoint, hue); 
  return 0;  
}

uint8_t zclGetSatCb(uint8_t sat, uint16_t nwkAddr, uint8_t endpoint)
{
  uprintf_default("\nzclGetSatCb:\n    Network Addr : 0x%04x\n    End Point    : 0x%02x\n    Saturation   : %02x\n\n", 
    nwkAddr, endpoint, sat); 
  return 0;  
}
