/*
 * zllSocCmd.c
 *
 * This module contains the API for the zll SoC Host Interface.
 *
 *
 */


/*********************************************************************
 * INCLUDES
 */
#include <termios.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <errno.h>

#include "ipc.h"
#include "assert.h"
#include "zllSocCmd.h"
#include "mt.h"
#include "hue.h"
#include "zll_controller.h"
#include "zcl_lighting.h"
#include "uprintf.h"

/*********************************************************************
 * MACROS
 */

#define APPCMDHEADER(len) \
0xFE,                                                                             \
len,   /*RPC payload Len                                      */     \
0x29, /*MT_RPC_CMD_AREQ + MT_RPC_SYS_APP        */     \
0x00, /*MT_APP_MSG                                                   */     \
0x0B, /*Application Endpoint                                  */     \
0x02, /*short Addr 0x0002                                     */     \
0x00, /*short Addr 0x0002                                     */     \
0x0B, /*Dst EP                                                             */     \
0xFF, /*Cluster ID 0xFFFF invalid, used for key */     \
0xFF, /*Cluster ID 0xFFFF invalid, used for key */     \

#define BUILD_UINT16(loByte, hiByte) \
          ((uint16_t)(((loByte) & 0x00FF) + (((hiByte) & 0x00FF) << 8)))
          
#define BUILD_UINT32(Byte0, Byte1, Byte2, Byte3) \
          ((uint32_t)((uint32_t)((Byte0) & 0x00FF) \
          + ((uint32_t)((Byte1) & 0x00FF) << 8) \
          + ((uint32_t)((Byte2) & 0x00FF) << 16) \
          + ((uint32_t)((Byte3) & 0x00FF) << 24)))
          
/*********************************************************************
 * CONSTANTS
 */
#define ZLL_MT_APP_RPC_CMD_TOUCHLINK          0x01
#define ZLL_MT_APP_RPC_CMD_RESET_TO_FN        0x02
#define ZLL_MT_APP_RPC_CMD_CH_CHANNEL         0x03
#define ZLL_MT_APP_RPC_CMD_JOIN_HA            0x04
#define ZLL_MT_APP_RPC_CMD_PERMIT_JOIN        0x05
#define ZLL_MT_APP_RPC_CMD_SEND_RESET_TO_FN   0x06

#define MT_APP_ZLL_NEW_DEV_IND               0x82

#define COMMAND_LIGHTING_MOVE_TO_HUE  0x00
#define COMMAND_LIGHTING_MOVE_TO_SATURATION 0x03
#define COMMAND_LEVEL_MOVE_TO_LEVEL 0x00

/*** Foundation Command IDs ***/
#define ZCL_CMD_READ                                    0x00
#define ZCL_CMD_READ_RSP                                0x01
#define ZCL_CMD_WRITE                                   0x02
#define ZCL_CMD_WRITE_UNDIVIDED                         0x03
#define ZCL_CMD_WRITE_RSP                               0x04

// General Clusters
#define ZCL_CLUSTER_ID_GEN_IDENTIFY                          0x0003
#define ZCL_CLUSTER_ID_GEN_GROUPS                            0x0004
#define ZCL_CLUSTER_ID_GEN_SCENES                            0x0005
#define ZCL_CLUSTER_ID_GEN_ON_OFF                            0x0006
#define ZCL_CLUSTER_ID_GEN_LEVEL_CONTROL                     0x0008
// Lighting Clusters
#define ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL                0x0300

// Data Types
#define ZCL_DATATYPE_BOOLEAN                            0x10
#define ZCL_DATATYPE_UINT8                              0x20
#define ZCL_DATATYPE_INT16                              0x29
#define ZCL_DATATYPE_INT24                              0x2a

/*******************************/
/*** Generic Cluster ATTR's  ***/
/*******************************/
#define ATTRID_ON_OFF                                     0x0000
#define ATTRID_LEVEL_CURRENT_LEVEL                        0x0000

/*******************************/
/*** Lighting Cluster ATTR's  ***/
/*******************************/
#define ATTRID_LIGHTING_COLOR_CONTROL_CURRENT_HUE         0x0000
#define ATTRID_LIGHTING_COLOR_CONTROL_CURRENT_SATURATION  0x0001


/*******************************/
/*** Scenes Cluster Commands ***/
/*******************************/
#define COMMAND_SCENE_STORE                               0x04
#define COMMAND_SCENE_RECALL                              0x05

/*******************************/
/*** Groups Cluster Commands ***/
/*******************************/
#define COMMAND_GROUP_ADD                                 0x00

/* The 3 MSB's of the 1st command field byte are for command type. */
#define MT_RPC_CMD_TYPE_MASK  0xE0

/* The 5 LSB's of the 1st command field byte are for the subsystem. */
#define MT_RPC_SUBSYSTEM_MASK 0x1F

#define MT_RPC_SOF         0xFE


/************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */

/*********************************************************************
 * LOCAL VARIABLES
 */
uint8_t transSeqNumber = 0;

/*********************************************************************
 * LOCAL FUNCTIONS
 */
void calcFcs(uint8_t *msg, int size);
static void processRpcSysAppTlInd(uint8_t *TlIndBuff);
static void processRpcSysAppNewDevInd(uint8_t *TlIndBuff);
static void processRpcSysAppZcl(uint8_t *zclRspBuff);
static void processRpcSysAppZclFoundation(uint8_t *zclRspBuff, uint8_t zclFrameLen, uint16_t clusterID, uint16_t nwkAddr, uint8_t endpoint);
static void processRpcSysApp(uint8_t *rpcBuff);
static void processRpcSysDbg(uint8_t *rpcBuff);

/*********************************************************************
 * @fn      calcFcs
 *
 * @brief   populates the Frame Check Sequence of the RPC payload.
 *
 * @param   msg - pointer to the RPC message
 *
 * @return  none
 */
void calcFcs(uint8_t *msg, int size)
{
	uint8_t result = 0;
	int idx = 1; //skip SOF
	int len = (size - 1);  // skip FCS
	
	while ((len--) != 0) {
		result ^= msg[idx++];
	}
	
	msg[(size-1)] = result;
}

/*********************************************************************
 * API FUNCTIONS
 */

extern mbox_t cdc_mbox;
extern int usbh_usr_cdc_txreq(uint8_t *buff, uint8_t length);

int zllctrl_write(int zll_fd, uint8_t *cmd, uint16_t cmd_len)
{
	uint32_t mail[2];
    int ret = -1;
    uint8_t *tx_buf;

    tx_buf = malloc(sizeof(cmd_len));
    if(tx_buf)
    {
        memcpy(tx_buf, cmd, cmd_len);
    	mail[0] = (/* CDC_MSG_TYPE_TXREQ */ 2 << 24) | (cmd_len << 16) | 0;
    	mail[1] = (uint32_t) tx_buf;
        ret = mbox_post(&cdc_mbox, mail);
        if(ret != ROK)
            free(tx_buf);
	}

	return ret;
}


/*********************************************************************
 * @fn      zllSocTouchLink
 *
 * @brief   Send the touchLink command to the CC253x.
 *
 * @param   none
 *
 * @return  none
 */
uint32_t zllSocTouchLink(uint8_t *cmbbuf)
{
	uint8_t tlCmd[] = {
		APPCMDHEADER(13)
		0x06, //Data Len
		0x02, //Address Mode
		0x00, //2dummy bytes
		0x00,
		ZLL_MT_APP_RPC_CMD_TOUCHLINK,
		0x00,     //
		0x00,     //
		0x00       //FCS - fill in later
    };
	
    calcFcs(tlCmd, sizeof(tlCmd));
    zllctrl_write(0,tlCmd, sizeof(tlCmd));

    if(cmbbuf)
      memcpy(cmbbuf, tlCmd, sizeof(tlCmd));
    return sizeof(tlCmd);
}

/*********************************************************************
 * @fn      zllSocResetToFn
 *
 * @brief   Send the reset to factory new command to the CC253x.
 *
 * @param   none
 *
 * @return  none
 */
uint32_t zllSocResetToFn(uint8_t *cmbbuf)
{  
	uint8_t tlCmd[] = {
		APPCMDHEADER(13)
		0x06, //Data Len
		0x02, //Address Mode
		0x00, //2dummy bytes
		0x00,
		ZLL_MT_APP_RPC_CMD_RESET_TO_FN,
		0x00,     //
		0x00,     //
		0x00       //FCS - fill in later
    };
	  
    calcFcs(tlCmd, sizeof(tlCmd));
    zllctrl_write(0,tlCmd, sizeof(tlCmd));

    if(cmbbuf)
      memcpy(cmbbuf, tlCmd, sizeof(tlCmd));
    return sizeof(tlCmd);
}

/*********************************************************************
 * @fn      zllSocSendResetToFn
 *
 * @brief   Send the reset to factory new command to a ZLL device.
 *
 * @param   none
 *
 * @return  none
 */
uint32_t zllSocSendResetToFn(uint8_t *cmbbuf)
{  
	uint8_t tlCmd[] = {
		APPCMDHEADER(13)
		0x06, //Data Len
		0x02, //Address Mode
		0x00, //2dummy bytes
		0x00,
		ZLL_MT_APP_RPC_CMD_SEND_RESET_TO_FN,
		0x00,     //
		0x00,     //
		0x00       //FCS - fill in later
    };
	  
    calcFcs(tlCmd, sizeof(tlCmd));
    zllctrl_write(0,tlCmd, sizeof(tlCmd));

    if(cmbbuf)
      memcpy(cmbbuf, tlCmd, sizeof(tlCmd));
    return sizeof(tlCmd);
}

/*********************************************************************
 * @fn      zllSocOpenNwk
 *
 * @brief   Send the open network command to a ZLL device.
 *
 * @param   none
 *
 * @return  none
 */
uint32_t zllSocOpenNwk(uint8_t *cmbbuf, uint8_t duration)
{  
	uint8_t cmd[] = {
		APPCMDHEADER(13)
		0x06, //Data Len
		0x02, //Address Mode
		0x00, //2dummy bytes
		0x00,
		ZLL_MT_APP_RPC_CMD_PERMIT_JOIN,
		duration, // open for duration sec, 0xFF is forever
		1,  // open all devices
		0x00  //FCS - fill in later
    };
	  
    calcFcs(cmd, sizeof(cmd));
    zllctrl_write(0,cmd, sizeof(cmd));

    if(cmbbuf)
      memcpy(cmbbuf, cmd, sizeof(cmd));
    return sizeof(cmd);
}

/*********************************************************************
 * @fn      zllSocSetState
 *
 * @brief   Send the on/off command to a ZLL light.
 *
 * @param   state - 0: Off, 1: On.
 * @param   dstAddr - Nwk Addr or Group ID of the Light(s) to be controled.
 * @param   endpoint - endpoint of the Light.
 * @param   addrMode - Unicast or Group cast.
 *
 * @return  none
 */
uint32_t zllSocSetState(uint8_t *cmbbuf, uint8_t state, uint16_t dstAddr, uint8_t endpoint, uint8_t addrMode)
{
  	uint8_t cmd[] = {
  		0xFE,                                                                                      
  		11,   /*RPC payload Len */          
  		0x29, /*MT_RPC_CMD_AREQ + MT_RPC_SYS_APP */          
  		0x00, /*MT_APP_MSG  */          
  		0x0B, /*Application Endpoint */          
  		(dstAddr & 0x00ff),
  		(dstAddr & 0xff00) >> 8,
  		endpoint, /*Dst EP */          
  		(ZCL_CLUSTER_ID_GEN_ON_OFF & 0x00ff),
  		(ZCL_CLUSTER_ID_GEN_ON_OFF & 0xff00) >> 8,
  		0x04, //Data Len
  		addrMode, 
  		0x01, //0x01 ZCL frame control field.  (send to the light cluster only)
  		transSeqNumber++,
		(state ? 1:0),
  		0x00       //FCS - fill in later
  	};
      
  	calcFcs(cmd, sizeof(cmd));
  	
    zllctrl_write(0,cmd,sizeof(cmd));

    if(cmbbuf)
      memcpy(cmbbuf, cmd, sizeof(cmd));
    return sizeof(cmd);
}

/*********************************************************************
 * @fn      zllSocSetLevel
 *
 * @brief   Send the level command to a ZLL light.
 *
 * @param   level - 0-128 = 0-100%
 * @param   dstAddr - Nwk Addr or Group ID of the Light(s) to be controled.
 * @param   endpoint - endpoint of the Light.
 * @param   addrMode - Unicast or Group cast.
 *
 * @return  size of command
 */
uint32_t zllSocSetLevel(uint8_t *cmbbuf, uint8_t level, uint16_t time, uint16_t dstAddr, uint8_t endpoint, uint8_t addrMode)
{
  	uint8_t cmd[] = {
  		0xFE,                                                                                                                                                                                    
  		14,   //RPC payload Len
  		0x29, //MT_RPC_CMD_AREQ + MT_RPC_SYS_APP
  		0x00, //MT_APP_MSG
  		0x0B, //Application Endpoint
  		(dstAddr & 0x00ff),
  		(dstAddr & 0xff00) >> 8,
  		endpoint, //Dst EP
  		(ZCL_CLUSTER_ID_GEN_LEVEL_CONTROL & 0x00ff),
  		(ZCL_CLUSTER_ID_GEN_LEVEL_CONTROL & 0xff00) >> 8,      
  		0x07, //Data Len
  		addrMode, 
  		0x01, //0x01 ZCL frame control field.  (send to the light cluster only)
  		transSeqNumber++,
  		COMMAND_LEVEL_MOVE_TO_LEVEL,
  		(level & 0xff),
  		(time & 0xff),
  		(time & 0xff00) >> 8,
  		0x00       //FCS - fill in later
  	};    
    
    calcFcs(cmd, sizeof(cmd));
    
    zllctrl_write(0,cmd,sizeof(cmd));

    if(cmbbuf)
      memcpy(cmbbuf, cmd, sizeof(cmd));
    return sizeof(cmd);
}

/*********************************************************************
 * @fn      zllSocSetHue
 *
 * @brief   Send the hue command to a ZLL light.
 *
 * @param   hue - 0-128 represent the 360Deg hue color wheel : 0=red, 42=blue, 85=green  
 * @param   dstAddr - Nwk Addr or Group ID of the Light(s) to be controled.
 * @param   endpoint - endpoint of the Light.
 * @param   addrMode - Unicast or Group cast.
 *
 * @return  none
 */
uint32_t zllSocSetHue(uint8_t *cmbbuf, uint8_t hue, uint16_t time, uint16_t dstAddr, uint8_t endpoint, uint8_t addrMode)
{
	uint8_t cmd[] = {
		0xFE,                                                                                                                                                                                    
		15,   //RPC payload Len          
		0x29, //MT_RPC_CMD_AREQ + MT_RPC_SYS_APP          
		0x00, //MT_APP_MSG          
		0x0B, //Application Endpoint          
		(dstAddr & 0x00ff),
		(dstAddr & 0xff00) >> 8,
		endpoint, //Dst EP
		(ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL & 0x00ff),
		(ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL & 0xff00) >> 8,     		   
		0x08, //Data Len
		addrMode, 
		0x01, //0x01 ZCL frame control field.  (send to the light cluster only)
		transSeqNumber++,
		COMMAND_LIGHTING_MOVE_TO_HUE,
		(hue & 0xff),
		0x00, //Move with shortest distance
		(time & 0xff),
		(time & 0xff00) >> 8,
		0x00       //FCS - fill in later
	};    

  calcFcs(cmd, sizeof(cmd));
  zllctrl_write(0,cmd,sizeof(cmd));

  if(cmbbuf)
    memcpy(cmbbuf, cmd, sizeof(cmd));
  return sizeof(cmd);    
}

/*********************************************************************
 * @fn      zllSocSetSat
 *
 * @brief   Send the satuartion command to a ZLL light.
 *
 * @param   sat - 0-128 : 0=white, 128: fully saturated color  
 * @param   dstAddr - Nwk Addr or Group ID of the Light(s) to be controled.
 * @param   endpoint - endpoint of the Light.
 * @param   addrMode - Unicast or Group cast.
 *
 * @return  none
 */
uint32_t zllSocSetSat(uint8_t *cmbbuf, uint8_t sat, uint16_t time, uint16_t dstAddr, uint8_t  endpoint, uint8_t addrMode)
{
  uint8_t cmd[] = {
		0xFE,                                                                                                                                                                                    
		14,   //RPC payload Len          
		0x29, //MT_RPC_CMD_AREQ + MT_RPC_SYS_APP         
		0x00, //MT_APP_MSG          
		0x0B, //Application Endpoint          
		(dstAddr & 0x00ff),
		(dstAddr & 0xff00) >> 8,
		endpoint, //Dst EP         
  	(ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL & 0x00ff),
  	(ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL & 0xff00) >> 8,
		0x07, //Data Len
		addrMode, 
		0x01, //0x01 ZCL frame control field.  (send to the light cluster only)
    transSeqNumber++,
		COMMAND_LIGHTING_MOVE_TO_SATURATION,
		(sat & 0xff),
		(time & 0xff),
		(time & 0xff00) >> 8,
		0x00       //FCS - fill in later
	};
	
	calcFcs(cmd, sizeof(cmd));
  zllctrl_write(0,cmd,sizeof(cmd));

  if(cmbbuf)
    memcpy(cmbbuf, cmd, sizeof(cmd));
  return sizeof(cmd);    
}

/*********************************************************************
 * @fn      zllSocSetHueSat
 *
 * @brief   Send the hue and satuartion command to a ZLL light.
 *
 * @param   hue - 0-128 represent the 360Deg hue color wheel : 0=red, 42=blue, 85=green  
 * @param   sat - 0-128 : 0=white, 128: fully saturated color  
 * @param   dstAddr - Nwk Addr or Group ID of the Light(s) to be controled.
 * @param   endpoint - endpoint of the Light.
 * @param   addrMode - Unicast or Group cast.
 *
 * @return  none
 */
uint32_t zllSocSetHueSat(uint8_t *cmbbuf, uint8_t hue, uint8_t sat, uint16_t time, uint16_t dstAddr, uint8_t endpoint, uint8_t addrMode)
{
	uint8_t cmd[] = { 
		0xFE, 
		15, //RPC payload Len
		0x29, //MT_RPC_CMD_AREQ + MT_RPC_SYS_APP
		0x00, //MT_APP_MSG
		0x0B, //Application Endpoint         
		(dstAddr & 0x00ff),
		(dstAddr & 0xff00) >> 8,
		endpoint, //Dst EP
  	(ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL & 0x00ff),
  	(ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL & 0xff00) >> 8,
		0x08, //Data Len
    addrMode,
		0x01, //ZCL Header Frame Control
		transSeqNumber++,
		0x06, //ZCL Header Frame Command (COMMAND_LEVEL_MOVE_TO_HUE_AND_SAT)
		hue, //HUE - fill it in later
		sat, //SAT - fill it in later
		(time & 0xff),
		(time & 0xff00) >> 8,
		0x00 //fcs
  }; 

  calcFcs(cmd, sizeof(cmd));
  zllctrl_write(0,cmd,sizeof(cmd));

  if(cmbbuf)
    memcpy(cmbbuf, cmd, sizeof(cmd));
  return sizeof(cmd);
}

/*********************************************************************
 * @fn      zllSocSetColor
 *
 * @brief   Send the set xy color command to a ZLL light.
 *
 * @param   x - 
 * @param   y -   
 * @param   dstAddr - Nwk Addr or Group ID of the Light(s) to be controled.
 * @param   endpoint - endpoint of the Light.
 * @param   addrMode - Unicast or Group cast.
 *
 * @return  none
 */
uint32_t zllSocSetColor(uint8_t *cmbbuf, uint16_t x, uint16_t y, uint16_t time, uint16_t dstAddr, uint8_t endpoint, uint8_t addrMode)
{
	uint8_t cmd[] = { 
		0xFE, 
		17, //RPC payload Len
		0x29, //MT_RPC_CMD_AREQ + MT_RPC_SYS_APP
		0x00, //MT_APP_MSG
		0x0B, //Application Endpoint         
		(dstAddr & 0x00ff),
		(dstAddr & 0xff00) >> 8,
		endpoint, //Dst EP
  	(ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL & 0x00ff),
  	(ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL & 0xff00) >> 8,
		0x0a, //Data Len
    addrMode,
		0x01, //ZCL Header Frame Control
		transSeqNumber++,
		COMMAND_LIGHTING_MOVE_TO_COLOR, //ZCL Header Frame Command (COMMAND_LEVEL_MOVE_TO_HUE_AND_SAT)
		(x & 0xff), //x
		(x & 0xff00) >> 8,
		(y & 0xff), //y
		(y & 0xff00) >> 8,
		(time & 0xff),
		(time & 0xff00) >> 8,
		0x00 //fcs
  }; 

  calcFcs(cmd, sizeof(cmd));
  zllctrl_write(0,cmd,sizeof(cmd));

  if(cmbbuf)
    memcpy(cmbbuf, cmd, sizeof(cmd));
  return sizeof(cmd);
}

/*********************************************************************
 * @fn      zllSocSetColorTemperature
 *
 * @brief   Send the set color temperature command to a ZLL light. (not supported!!!)
 *
 * @param   ct - 
 * @param   dstAddr - Nwk Addr or Group ID of the Light(s) to be controled.
 * @param   endpoint - endpoint of the Light.
 * @param   addrMode - Unicast or Group cast.
 *
 * @return  none
 */


/*********************************************************************
 * @fn      zllSocAddGroup
 *
 * @brief   Add Group.
 *
 * @param   groupId - Group ID of the Scene.
 * @param   dstAddr - Nwk Addr or Group ID of the Light(s) to be controled.
 * @param   endpoint - endpoint of the Light.
 * @param   addrMode - Unicast or Group cast. 
 *
 * @return  none
 */
uint32_t zllSocAddGroup(uint8_t *cmbbuf, uint16_t groupId, uint16_t dstAddr, uint8_t endpoint, uint8_t addrMode)
{  
	uint8_t cmd[] = {
		0xFE,                                                                                      
		14,   /*RPC payload Len */          
		0x29, /*MT_RPC_CMD_AREQ + MT_RPC_SYS_APP */          
		0x00, /*MT_APP_MSG  */          
		0x0B, /*Application Endpoint */          
		(dstAddr & 0x00ff),
		(dstAddr & 0xff00) >> 8,
		endpoint, /*Dst EP */          
		(ZCL_CLUSTER_ID_GEN_GROUPS & 0x00ff),
		(ZCL_CLUSTER_ID_GEN_GROUPS & 0xff00) >> 8,
		0x07, //Data Len
		addrMode, 
		0x01, //0x01 ZCL frame control field.  (send to the light cluster only)
		transSeqNumber++,
		COMMAND_GROUP_ADD,
 		(groupId & 0x00ff),
		(groupId & 0xff00) >> 8, 
		0, //Null group name - Group Name not pushed to the devices	
		0x00       //FCS - fill in later
	};
	
	uprintf_default("zllSocAddGroup: dstAddr 0x%x\n", dstAddr);
    
	calcFcs(cmd, sizeof(cmd));
	
  zllctrl_write(0,cmd,sizeof(cmd));

  if(cmbbuf)
    memcpy(cmbbuf, cmd, sizeof(cmd));
  return sizeof(cmd);
}

/*********************************************************************
 * @fn      zllSocStoreScene
 *
 * @brief   Store Scene.
 * 
 * @param   groupId - Group ID of the Scene.
 * @param   sceneId - Scene ID of the Scene.
 * @param   dstAddr - Nwk Addr or Group ID of the Light(s) to be controled.
 * @param   endpoint - endpoint of the Light.
 * @param   addrMode - Unicast or Group cast. 
 *
 * @return  none
 */
uint32_t zllSocStoreScene(uint8_t *cmbbuf, uint16_t groupId, uint8_t sceneId, uint16_t dstAddr, uint8_t endpoint, uint8_t addrMode)
{  
	uint8_t cmd[] = {
		0xFE,                                                                                      
		14,   /*RPC payload Len */          
		0x29, /*MT_RPC_CMD_AREQ + MT_RPC_SYS_APP */          
		0x00, /*MT_APP_MSG  */          
		0x0B, /*Application Endpoint */          
		(dstAddr & 0x00ff),
		(dstAddr & 0xff00) >> 8,
		endpoint, /*Dst EP */          
		(ZCL_CLUSTER_ID_GEN_SCENES & 0x00ff),
		(ZCL_CLUSTER_ID_GEN_SCENES & 0xff00) >> 8,
		0x07, //Data Len
		addrMode, 
		0x01, //0x01 ZCL frame control field.  (send to the light cluster only)
		transSeqNumber++,
		COMMAND_SCENE_STORE,
 		(groupId & 0x00ff),
		(groupId & 0xff00) >> 8, 	
		sceneId++,	
		0x00       //FCS - fill in later
	};
    
	calcFcs(cmd, sizeof(cmd));
	
  zllctrl_write(0,cmd,sizeof(cmd));

  if(cmbbuf)
    memcpy(cmbbuf, cmd, sizeof(cmd));
  return sizeof(cmd);
}

/*********************************************************************
 * @fn      zllSocRecallScene
 *
 * @brief   Recall Scene.
 *
 * @param   groupId - Group ID of the Scene.
 * @param   sceneId - Scene ID of the Scene.
 * @param   dstAddr - Nwk Addr or Group ID of the Light(s) to be controled. 
 * @param   endpoint - endpoint of the Light.
 * @param   addrMode - Unicast or Group cast. 
 
 * @return  none
 */
uint32_t zllSocRecallScene(uint8_t *cmbbuf, uint16_t groupId, uint8_t sceneId, uint16_t dstAddr, uint8_t endpoint, uint8_t addrMode)
{  
	uint8_t cmd[] = {
		0xFE,                                                                                      
		14,   /*RPC payload Len */          
		0x29, /*MT_RPC_CMD_AREQ + MT_RPC_SYS_APP */          
		0x00, /*MT_APP_MSG  */          
		0x0B, /*Application Endpoint */          
		(dstAddr & 0x00ff),
		(dstAddr & 0xff00) >> 8,
		endpoint, /*Dst EP */          
		(ZCL_CLUSTER_ID_GEN_SCENES & 0x00ff),
		(ZCL_CLUSTER_ID_GEN_SCENES & 0xff00) >> 8,
		0x07, //Data Len
		addrMode, 
		0x01, //0x01 ZCL frame control field.  (send to the light cluster only)
		transSeqNumber++,
		COMMAND_SCENE_RECALL,
 		(groupId & 0x00ff),
		(groupId & 0xff00) >> 8, 	
		sceneId++,	
		0x00       //FCS - fill in later
	};
    
	calcFcs(cmd, sizeof(cmd));
	
  zllctrl_write(0,cmd,sizeof(cmd));

  if(cmbbuf)
    memcpy(cmbbuf, cmd, sizeof(cmd));
  return sizeof(cmd);
}

/*********************************************************************
 * @fn      zllSocBind
 *
 * @brief   Recall Scene.
 *
 * @param   
 *
 * @return  none
 */
uint32_t zllSocBind(uint8_t *cmbbuf, uint16_t srcNwkAddr, uint8_t srcEndpoint, uint8_t srcIEEE[8], uint8_t dstEndpoint, uint8_t dstIEEE[8], uint16_t clusterID )
{  
	uint8_t cmd[] = {
		0xFE,                                                                                      
		23,                           /*RPC payload Len */          
		0x25,                         /*MT_RPC_CMD_SREQ + MT_RPC_SYS_ZDO */        
		0x21,                         /*MT_ZDO_BIND_REQ*/        
  	(srcNwkAddr & 0x00ff),        /*Src Nwk Addr - To send the bind message to*/
  	(srcNwkAddr & 0xff00) >> 8,   /*Src Nwk Addr - To send the bind message to*/
  	srcIEEE[0],                   /*Src IEEE Addr for the binding*/
  	srcIEEE[1],                   /*Src IEEE Addr for the binding*/
  	srcIEEE[2],                   /*Src IEEE Addr for the binding*/
  	srcIEEE[3],                   /*Src IEEE Addr for the binding*/
  	srcIEEE[4],                   /*Src IEEE Addr for the binding*/
  	srcIEEE[5],                   /*Src IEEE Addr for the binding*/
  	srcIEEE[6],                   /*Src IEEE Addr for the binding*/
  	srcIEEE[7],                   /*Src IEEE Addr for the binding*/ 	
  	srcEndpoint,                  /*Src endpoint for the binding*/ 
  	(clusterID & 0x00ff),         /*cluster ID to bind*/
  	(clusterID & 0xff00) >> 8,    /*cluster ID to bind*/  	
  	afAddr64Bit,                    /*Addr mode of the dst to bind*/
  	dstIEEE[0],                   /*Dst IEEE Addr for the binding*/
  	dstIEEE[1],                   /*Dst IEEE Addr for the binding*/
  	dstIEEE[2],                   /*Dst IEEE Addr for the binding*/
  	dstIEEE[3],                   /*Dst IEEE Addr for the binding*/
  	dstIEEE[4],                   /*Dst IEEE Addr for the binding*/
  	dstIEEE[5],                   /*Dst IEEE Addr for the binding*/
  	dstIEEE[6],                   /*Dst IEEE Addr for the binding*/
  	dstIEEE[7],                   /*Dst IEEE Addr for the binding*/ 	
  	dstEndpoint,                  /*Dst endpoint for the binding*/  	  	
		0x00       //FCS - fill in later
	};
      
	calcFcs(cmd, sizeof(cmd));
	
	uprintf_default("zllSocBind: srcNwkAddr=0x%x, srcEndpoint=0x%x, srcIEEE=0x%x:%x:%x:%x:%x:%x:%x:%x, dstEndpoint=0x%x, dstIEEE=0x%x:%x:%x:%x:%x:%x:%x:%x, clusterID:%x\n", 
	          srcNwkAddr, srcEndpoint, srcIEEE[0], srcIEEE[1], srcIEEE[2], srcIEEE[3], srcIEEE[4], srcIEEE[5], srcIEEE[6], srcIEEE[7], 
	          srcEndpoint, dstIEEE[0], dstIEEE[1], dstIEEE[2], dstIEEE[3], dstIEEE[4], dstIEEE[5], dstIEEE[6], dstIEEE[7], clusterID);
	
  zllctrl_write(0,cmd,sizeof(cmd));

  if(cmbbuf)
    memcpy(cmbbuf, cmd, sizeof(cmd));
  return sizeof(cmd);
}

/*********************************************************************
 * @fn      zllSocGetState
 *
 * @brief   Send the get state command to a ZLL light.
 *
 * @param   dstAddr - Nwk Addr or Group ID of the Light(s) to be sent the command.
 * @param   endpoint - endpoint of the Light.
 * @param   addrMode - Unicast or Group cast.
 *
 * @return  none
 */
uint32_t zllSocGetState(uint8_t *cmbbuf, uint16_t dstAddr, uint8_t endpoint, uint8_t addrMode)
{  	  
  	uint8_t cmd[] = {
  		0xFE,                                                                                      
  		13,   /*RPC payload Len */          
  		0x29, /*MT_RPC_CMD_AREQ + MT_RPC_SYS_APP */          
  		0x00, /*MT_APP_MSG  */          
  		0x0B, /*Application Endpoint */          
  		(dstAddr & 0x00ff),
  		(dstAddr & 0xff00) >> 8,
  		endpoint, /*Dst EP */          
  		(ZCL_CLUSTER_ID_GEN_ON_OFF & 0x00ff),
  		(ZCL_CLUSTER_ID_GEN_ON_OFF & 0xff00) >> 8,
  		0x06, //Data Len
  		addrMode, 
  		0x00, //0x00 ZCL frame control field.  not specific to a cluster (i.e. a SCL founadation command)
  		transSeqNumber++,
  		ZCL_CMD_READ,
  		(ATTRID_ON_OFF & 0x00ff),
  		(ATTRID_ON_OFF & 0xff00) >> 8,
  		0x00       //FCS - fill in later
  	};
      
  	calcFcs(cmd, sizeof(cmd));
  	
    zllctrl_write(0,cmd,sizeof(cmd));

    if(cmbbuf)
      memcpy(cmbbuf, cmd, sizeof(cmd));
    return sizeof(cmd);
} 
 
/*********************************************************************
 * @fn      zllSocGetLevel
 *
 * @brief   Send the get level command to a ZLL light.
 *
 * @param   dstAddr - Nwk Addr or Group ID of the Light(s) to be sent the command.
 * @param   endpoint - endpoint of the Light.
 * @param   addrMode - Unicast or Group cast.
 *
 * @return  none
 */
uint32_t zllSocGetLevel(uint8_t *cmbbuf, uint16_t dstAddr, uint8_t endpoint, uint8_t addrMode)
{
  	uint8_t cmd[] = {
  		0xFE,                                                                                      
  		13,   /*RPC payload Len */          
  		0x29, /*MT_RPC_CMD_AREQ + MT_RPC_SYS_APP */          
  		0x00, /*MT_APP_MSG  */          
  		0x0B, /*Application Endpoint */          
  		(dstAddr & 0x00ff),
  		(dstAddr & 0xff00) >> 8,
  		endpoint, /*Dst EP */          
  		(ZCL_CLUSTER_ID_GEN_LEVEL_CONTROL & 0x00ff),
  		(ZCL_CLUSTER_ID_GEN_LEVEL_CONTROL & 0xff00) >> 8,
  		0x06, //Data Len
  		addrMode, 
  		0x00, //0x00 ZCL frame control field.  not specific to a cluster (i.e. a SCL founadation command)
  		transSeqNumber++,
  		ZCL_CMD_READ,
  		(ATTRID_LEVEL_CURRENT_LEVEL & 0x00ff),
  		(ATTRID_LEVEL_CURRENT_LEVEL & 0xff00) >> 8,
  		0x00       //FCS - fill in later
  	};
      
  	calcFcs(cmd, sizeof(cmd));
  	
    zllctrl_write(0,cmd,sizeof(cmd));

    if(cmbbuf)
      memcpy(cmbbuf, cmd, sizeof(cmd));
    return sizeof(cmd);
} 

/*********************************************************************
 * @fn      zllSocGetHue
 *
 * @brief   Send the get hue command to a ZLL light.
 *
 * @param   dstAddr - Nwk Addr or Group ID of the Light(s) to be sent the command.
 * @param   endpoint - endpoint of the Light.
 * @param   addrMode - Unicast or Group cast.
 *
 * @return  none
 */
uint32_t zllSocGetHue(uint8_t *cmbbuf, uint16_t dstAddr, uint8_t endpoint, uint8_t addrMode)
{
  	uint8_t cmd[] = {
  		0xFE,                                                                                      
  		13,   /*RPC payload Len */          
  		0x29, /*MT_RPC_CMD_AREQ + MT_RPC_SYS_APP */          
  		0x00, /*MT_APP_MSG  */          
  		0x0B, /*Application Endpoint */          
  		(dstAddr & 0x00ff),
  		(dstAddr & 0xff00) >> 8,
  		endpoint, /*Dst EP */          
  		(ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL & 0x00ff),
  		(ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL & 0xff00) >> 8,
  		0x06, //Data Len
  		addrMode, 
  		0x00, //0x00 ZCL frame control field.  not specific to a cluster (i.e. a SCL founadation command)
  		transSeqNumber++,
  		ZCL_CMD_READ,
  		(ATTRID_LIGHTING_COLOR_CONTROL_CURRENT_HUE & 0x00ff),
  		(ATTRID_LIGHTING_COLOR_CONTROL_CURRENT_HUE & 0xff00) >> 8,
  		0x00       //FCS - fill in later
  	};
      
  	calcFcs(cmd, sizeof(cmd));
  	
    zllctrl_write(0,cmd,sizeof(cmd));

    if(cmbbuf)
      memcpy(cmbbuf, cmd, sizeof(cmd));
    return sizeof(cmd);
} 

/*********************************************************************
 * @fn      zllSocGetSat
 *
 * @brief   Send the get saturation command to a ZLL light.
 *
 * @param   dstAddr - Nwk Addr or Group ID of the Light(s) to be sent the command.
 * @param   endpoint - endpoint of the Light.
 * @param   addrMode - Unicast or Group cast.
 *
 * @return  none
 */
uint32_t zllSocGetSat(uint8_t *cmbbuf, uint16_t dstAddr, uint8_t endpoint, uint8_t addrMode)
{
  	uint8_t cmd[] = {
  		0xFE,                                                                                      
  		13,   /*RPC payload Len */          
  		0x29, /*MT_RPC_CMD_AREQ + MT_RPC_SYS_APP */          
  		0x00, /*MT_APP_MSG  */          
  		0x0B, /*Application Endpoint */          
  		(dstAddr & 0x00ff),
  		(dstAddr & 0xff00) >> 8,
  		endpoint, /*Dst EP */          
  		(ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL & 0x00ff),
  		(ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL & 0xff00) >> 8,
  		0x06, //Data Len
  		addrMode, 
  		0x00, //0x00 ZCL frame control field.  not specific to a cluster (i.e. a SCL founadation command)
  		transSeqNumber++,
  		ZCL_CMD_READ,
  		(ATTRID_LIGHTING_COLOR_CONTROL_CURRENT_SATURATION & 0x00ff),
  		(ATTRID_LIGHTING_COLOR_CONTROL_CURRENT_SATURATION & 0xff00) >> 8,
  		0x00       //FCS - fill in later
  	};
      
  	calcFcs(cmd, sizeof(cmd));
  	
    zllctrl_write(0,cmd,sizeof(cmd));

    if(cmbbuf)
      memcpy(cmbbuf, cmd, sizeof(cmd));
    return sizeof(cmd);
} 

/*********************************************************************
 * @fn      zllSocSysResetReq
 *
 * @brief   Send the system reset req command (AREQ) to cc253x.
 *
 * @param   type - 0 hw reset, otherwise sw reset
 *
 * @return  none
 */
uint32_t zllSocSysResetReq(uint8_t *cmbbuf, uint8_t type)
{
  	uint8_t cmd[] = {
  		0xFE,                                                                                      
  		01,   /*RPC payload Len */          
  		0x41, /* cmd0 */          
  		0x00, /* cmd1: MT_SYS_RESET_REQ  */
  		type,
  		0x00       //FCS - fill in later
  	};
      
  	calcFcs(cmd, sizeof(cmd));
  	
    zllctrl_write(0,cmd,sizeof(cmd));

    if(cmbbuf)
      memcpy(cmbbuf, cmd, sizeof(cmd));
    return sizeof(cmd);
} 

/*********************************************************************
 * @fn      zllSocSysPing
 *
 * @brief   Send the system ping command to cc253x.
 *
 * @param   none
 *
 * @return  none
 */
uint32_t zllSocSysPing(uint8_t *cmbbuf)
{
	uint8_t cmd[] = {
		0xFE,
		0x00, /* length */
		0x21, /* cmd0 */
		0x01, /* cmd1: MT_SYS_PING */
		0x00  /* FCS */
	};

	calcFcs(cmd, sizeof(cmd));

	zllctrl_write(0, cmd, sizeof(cmd));

    if(cmbbuf)
      memcpy(cmbbuf, cmd, sizeof(cmd));
    return sizeof(cmd);
}


/*********************************************************************
 * @fn      zllSocSysVersion
 *
 * @brief   Send the system version command to cc253x.
 *
 * @param   none
 *
 * @return  none
 */
uint32_t zllSocSysVersion(uint8_t *cmbbuf)
{
	uint8_t cmd[] = {
		0xFE,
		0x00, /* length */
		0x21, /* cmd0 */
		0x02, /* cmd1: MT_SYS_VERSION */
		0x00  /* FCS */
	};

	calcFcs(cmd, sizeof(cmd));

	zllctrl_write(0, cmd, sizeof(cmd));

    if(cmbbuf)
      memcpy(cmbbuf, cmd, sizeof(cmd));
    return sizeof(cmd);
}

/*********************************************************************
 * @fn      zllSocSysSetExtAddr
 *
 * @brief   Send set extended address command to cc253x.
 *
 * @param   extAddr - IEEE address
 *
 * @return  none
 */
uint32_t zllSocSysSetExtAddr(uint8_t *cmbbuf, uint8_t *extAddr)
{
	uint8_t cmd[] = {
		0xFE,
		0x08, /* length */
		0x21, /* cmd0 */
		0x03, /* cmd1: MT_SYS_SET_EXTADDR */
		extAddr[0],
		extAddr[1],
		extAddr[2],
		extAddr[3],
		extAddr[4],
		extAddr[5],
		extAddr[6],
		extAddr[7],
		0x00  /* FCS */
	};

	calcFcs(cmd, sizeof(cmd));

	zllctrl_write(0, cmd, sizeof(cmd));

    if(cmbbuf)
      memcpy(cmbbuf, cmd, sizeof(cmd));
    return sizeof(cmd);
}

/*********************************************************************
 * @fn      zllSocSysGetExtAddr
 *
 * @brief   Send get extended address command to cc253x.
 *
 * @param   none
 *
 * @return  none
 */
uint32_t zllSocSysGetExtAddr(uint8_t *cmbbuf)
{
	uint8_t cmd[] = {
		0xFE,
		0x00, /* length */
		0x21, /* cmd0 */
		0x04, /* cmd1: MT_SYS_GET_EXTADDR */
		0x00  /* FCS */
	};

	calcFcs(cmd, sizeof(cmd));

	zllctrl_write(0, cmd, sizeof(cmd));

    if(cmbbuf)
      memcpy(cmbbuf, cmd, sizeof(cmd));
    return sizeof(cmd);
}

/*********************************************************************
 * @fn      zllSocSysRamRead
 *
 * @brief   Send read RAM command to cc253x.
 *
 * @param   addr - address to read
 * @param   len - length of read
 *
 * @return  none
 */
uint32_t zllSocSysRamRead(uint8_t *cmbbuf, uint16_t addr, uint8_t len)
{
	uint8_t cmd[] = {
		0xFE,
		0x03, /* length */
		0x21, /* cmd0 */
		0x05, /* cmd1: MT_SYS_RAM_READ */
		(addr & 0xFF),
		(addr & 0xFF00)>>8,
		len,
		0x00  /* FCS */
	};

	calcFcs(cmd, sizeof(cmd));

	zllctrl_write(0, cmd, sizeof(cmd));

    if(cmbbuf)
      memcpy(cmbbuf, cmd, sizeof(cmd));
    return sizeof(cmd);
}

/*********************************************************************
 * @fn      zllSocSysRamWrite
 *
 * @brief   Send write RAM command to cc253x.
 *
 * @param   addr - address to write
 * @param   len - length of write
 * @param   value - value of write
 *
 * @return  none
 */
uint32_t zllSocSysRamWrite(uint8_t *cmbbuf, uint16_t addr, uint8_t len, uint8_t *value)
{
	uint8_t cmd[SOC_MT_CMD_BUF_SIZ];
	uint8_t i=0;

	assert(len+7<SOC_MT_CMD_BUF_SIZ);

	cmd[i++] = 0xFE;
	cmd[i++] = 0x03+len; /* length */
	cmd[i++] = 0x21; /* cmd0 */
	cmd[i++] = 0x06; /* cmd1: MT_SYS_RAM_WRITE */
	cmd[i++] = (addr & 0xFF);
	cmd[i++] = (addr & 0xFF00)>>8;
	cmd[i++] = len;

	memcpy(&cmd[i], value, len);

	calcFcs(cmd, len+7);

	zllctrl_write(0, cmd, sizeof(cmd));

    if(cmbbuf)
      memcpy(cmbbuf, cmd, sizeof(cmd));
    return sizeof(cmd);
}

/*********************************************************************
 * @fn      zllSocSysNvInit
 *
 * @brief   Send non-volatile memory init command to cc253x.
 *             create and initialize an item
 *
 * @param   itemId - 
 * @param   itemLen - length of item
 * @param   initLen - init length
 * @param   initData -
 *
 * @return  none
 */
uint32_t zllSocSysNvInit(uint8_t *cmbbuf, uint16_t itemId, uint16_t itemLen, uint8_t initLen, uint8_t *initData)
{
	uint8_t cmd[SOC_MT_CMD_BUF_SIZ];
	uint8_t i=0;

	assert(initLen+9<SOC_MT_CMD_BUF_SIZ);

	cmd[i++] = 0xFE;
	cmd[i++] = 0x05+initLen; /* length */
	cmd[i++] = 0x21; /* cmd0 */
	cmd[i++] = 0x07; /* cmd1: MT_SYS_OSAL_NV_ITEM_INIT */
	cmd[i++] = (itemId & 0xFF);
	cmd[i++] = (itemId & 0xFF00)>>8;
	cmd[i++] = (itemLen & 0xFF);
	cmd[i++] = (itemLen & 0xFF00)>>8;
	cmd[i++] = initLen;

	if(initLen > 0)
		memcpy(&cmd[i], initData, initLen);

	calcFcs(cmd, initLen+9);

	zllctrl_write(0, cmd, sizeof(cmd));

    if(cmbbuf)
      memcpy(cmbbuf, cmd, sizeof(cmd));
    return sizeof(cmd);
}

/*********************************************************************
 * @fn      zllSocSysNVRead
 *
 * @brief   Send read NVRAM command to cc253x.
 *
 * @param   itemId - 
 * @param   offset - 
 *
 * @return  none
 */
uint32_t zllSocSysNVRead(uint8_t *cmbbuf, uint16_t itemId, uint8_t offset)
{
	uint8_t cmd[] = {
		0xFE,
		0x03, /* length */
		0x21, /* cmd0 */
		0x08, /* cmd1: MT_SYS_OSAL_NV_READ */
		(itemId & 0xFF),
		(itemId & 0xFF00)>>8,
		offset,
		0x00  /* FCS */
	};

	calcFcs(cmd, sizeof(cmd));

	zllctrl_write(0, cmd, sizeof(cmd));

    if(cmbbuf)
      memcpy(cmbbuf, cmd, sizeof(cmd));
    return sizeof(cmd);
}

/*********************************************************************
 * @fn      zllSocSysNVWrite
 *
 * @brief   Send write NVRAM command to cc253x.
 *
 * @param   itemId - 
 * @param   offset - 
 * @param   len - length of write
 * @param   value - value of write
 *
 * @return  none
 */
uint32_t zllSocSysNvWrite(uint8_t *cmbbuf, uint16_t itemId, uint8_t offset, uint8_t len, uint8_t *value)
{
	uint8_t cmd[SOC_MT_CMD_BUF_SIZ];
	uint8_t i=0;

	assert(len+8<SOC_MT_CMD_BUF_SIZ);

	cmd[i++] = 0xFE;
	cmd[i++] = 0x04+len; /* length */
	cmd[i++] = 0x21; /* cmd0 */
	cmd[i++] = 0x09; /* cmd1: MT_SYS_OSAL_NV_WRITE */
	cmd[i++] = (itemId & 0xFF);
	cmd[i++] = (itemId & 0xFF00)>>8;
	cmd[i++] = offset;
	cmd[i++] = len;

	memcpy(&cmd[i], value, len);

	calcFcs(cmd, len+8);

	zllctrl_write(0, cmd, sizeof(cmd));

    if(cmbbuf)
      memcpy(cmbbuf, cmd, sizeof(cmd));
    return sizeof(cmd);
}

/*********************************************************************
 * @fn      zllSocSysNVDelete
 *
 * @brief   Send delete NVRAM command to cc253x.
 *
 * @param   itemId - 
 * @param   itemLen - The ItemLen parameter must match the length of the NV item or the command will fail.
 *
 * @return  none
 */
uint32_t zllSocSysNVDelete(uint8_t *cmbbuf, uint16_t itemId, uint16_t itemLen)
{
	uint8_t cmd[] = {
		0xFE,
		0x04, /* length */
		0x21, /* cmd0 */
		0x12, /* cmd1: MT_SYS_OSAL_NV_DELETE */
		(itemId & 0xFF),
		(itemId & 0xFF00)>>8,
		(itemLen & 0xFF),
		(itemLen & 0xFF00)>>8,
		0x00  /* FCS */
	};

	calcFcs(cmd, sizeof(cmd));

	zllctrl_write(0, cmd, sizeof(cmd));

    if(cmbbuf)
      memcpy(cmbbuf, cmd, sizeof(cmd));
    return sizeof(cmd);
}

/*********************************************************************
 * @fn      zllSocSysNVLength
 *
 * @brief   Send get NVRAM length command to cc253x.
 *
 * @param   itemId - 
 *
 * @return  none
 */
uint32_t zllSocSysNVLength(uint8_t *cmbbuf, uint16_t itemId)
{
	uint8_t cmd[] = {
		0xFE,
		0x02, /* length */
		0x21, /* cmd0 */
		0x13, /* cmd1: MT_SYS_OSAL_NV_LENGTH */
		(itemId & 0xFF),
		(itemId & 0xFF00)>>8,
		0x00  /* FCS */
	};

	calcFcs(cmd, sizeof(cmd));

	zllctrl_write(0, cmd, sizeof(cmd));

    if(cmbbuf)
      memcpy(cmbbuf, cmd, sizeof(cmd));
    return sizeof(cmd);
}

/*********************************************************************
 * @fn      zllSocSysSetTxPower
 *
 * @brief   Send set Tx Power command to cc253x.
 *
 * @param   power - Actual TX power setting, in dBm.
 *
 * @return  none
 */
uint32_t zllSocSysSetTxPower(uint8_t *cmbbuf, uint8_t power)
{
	uint8_t cmd[] = {
		0xFE,
		0x01, /* length */
		0x21, /* cmd0 */
		0x14, /* cmd1: MT_SYS_SET_TX_POWER */
		power,
		0x00  /* FCS */
	};

	calcFcs(cmd, sizeof(cmd));

	zllctrl_write(0, cmd, sizeof(cmd));

    if(cmbbuf)
      memcpy(cmbbuf, cmd, sizeof(cmd));
    return sizeof(cmd);
}

/*********************************************************************
 * @fn      zllSocUtilGetDevInfo
 *
 * @brief   Send get device info command to cc253x.
 *
 * @param  none
 *
 * @return  none
 */
uint32_t zllSocUtilGetDevInfo(uint8_t *cmbbuf)
{
	uint8_t cmd[] = {
		0xFE,
		0x00, /* length */
		0x27, /* cmd0 */
		0x00, /* cmd1:  UTIL_GET_DEVICE_INFO */
		0x00  /* FCS */
	};

	calcFcs(cmd, sizeof(cmd));

	zllctrl_write(0, cmd, sizeof(cmd));

    if(cmbbuf)
      memcpy(cmbbuf, cmd, sizeof(cmd));
    return sizeof(cmd);
}

/*************************************************************************************************
 * @fn      processRpcSysAppTlInd()
 *
 * @brief  process the TL Indication from the ZLL controller
 *
 * @param   none
 *
 * @return  length of current Rx Buffer
 *************************************************************************************************/
static void processRpcSysAppTlInd(uint8_t *TlIndBuff)
{
  epInfo_t epInfo;    
    
  epInfo.nwkAddr = BUILD_UINT16(TlIndBuff[0], TlIndBuff[1]);
  TlIndBuff+=2;      
  epInfo.endpoint = *TlIndBuff++;
  epInfo.profileID = BUILD_UINT16(TlIndBuff[0], TlIndBuff[1]);
  TlIndBuff+=2;      
  epInfo.deviceID = BUILD_UINT16(TlIndBuff[0], TlIndBuff[1]);
  TlIndBuff+=2;   
  epInfo.version = *TlIndBuff++;
  epInfo.status = *TlIndBuff++;
  
  zllctrl_process_touchlink_indication(&epInfo);
}        

/*************************************************************************************************
 * @fn      processRpcSysAppNewDevInd()
 *
 * @brief  process the New Device Indication from the ZLL controller
 *
 * @param   none
 *
 * @return  length of current Rx Buffer
 *************************************************************************************************/
static void processRpcSysAppNewDevInd(uint8_t *TlIndBuff)
{
  epInfo_t epInfo;
  uint8_t i;    
    
  epInfo.status = 0;
  
  epInfo.nwkAddr = BUILD_UINT16(TlIndBuff[0], TlIndBuff[1]);
  TlIndBuff+=2;      
  epInfo.endpoint = *TlIndBuff++;
  epInfo.profileID = BUILD_UINT16(TlIndBuff[0], TlIndBuff[1]);
  TlIndBuff+=2;      
  epInfo.deviceID = BUILD_UINT16(TlIndBuff[0], TlIndBuff[1]);
  TlIndBuff+=2;   
  epInfo.version = *TlIndBuff++;
  
  for(i=0; i<8; i++)
  {
    epInfo.IEEEAddr[i] = *TlIndBuff++;
  }
  
  zllctrl_process_newdev_indication(&epInfo);
}

/*************************************************************************************************
 * @fn      processRpcSysAppZcl()
 *
 * @brief  process the ZCL Rsp from the ZLL controller
 *
 * @param   none
 *
 * @return  length of current Rx Buffer
 *************************************************************************************************/
static void processRpcSysAppZcl(uint8_t *zclRspBuff)
{   
  uint8_t zclHdrLen = 3;
  uint16_t nwkAddr, clusterID; 
  uint8_t endpoint, appEP, zclFrameLen, zclFrameFrameControl;
    
  uprintf_default("processRpcSysAppZcl++\n");
    
  //This is a ZCL response
  appEP = *zclRspBuff++;
  nwkAddr = BUILD_UINT16(zclRspBuff[0], zclRspBuff[1]);
  zclRspBuff+=2;

  endpoint = *zclRspBuff++;
  clusterID = BUILD_UINT16(zclRspBuff[0], zclRspBuff[1]);
  zclRspBuff+=2;

  zclFrameLen = *zclRspBuff++;
  zclFrameFrameControl = *zclRspBuff++;
  //is it manufacturer specific
  if ( zclFrameFrameControl & (1 <<2) )
  {
    //currently not supported shown for reference
    uint16_t ManSpecCode;
    //manu spec code
    ManSpecCode = BUILD_UINT16(zclRspBuff[0], zclRspBuff[1]);
    zclRspBuff+=2;
    //Manufacturer specif commands have 2 extra byte in te header
    zclHdrLen+=2;
  }      
  
  //is this a foundation command
  if( (zclFrameFrameControl & 0x3) == 0)
  {
    uprintf_default("processRpcSysAppZcl: Foundation messagex\n");
    processRpcSysAppZclFoundation(zclRspBuff, zclFrameLen, clusterID, nwkAddr, endpoint);
  }
}
    
/*************************************************************************************************
 * @fn      processRpcSysAppZclFoundation()
 *
 * @brief  process the ZCL Rsp from the ZLL controller
 *
 * @param   none
 *
 * @return  length of current Rx Buffer
 *************************************************************************************************/
static void processRpcSysAppZclFoundation(uint8_t *zclRspBuff, uint8_t zclFrameLen, uint16_t clusterID, uint16_t nwkAddr, uint8_t endpoint)
{
  uint8_t transSeqNum, commandID;
  
  transSeqNum = *zclRspBuff++;
  commandID = *zclRspBuff++;
  
  if(commandID == ZCL_CMD_READ_RSP)
  {
    uint16_t attrID;
    uint8_t status;
    uint8_t dataType;
    
    attrID = BUILD_UINT16(zclRspBuff[0], zclRspBuff[1]);      
    zclRspBuff+=2;
    status = *zclRspBuff++;
    //get data type;
    dataType = *zclRspBuff++;


    uprintf(UPRINT_INFO, UPRINT_BLK_HUE, "processRpcSysAppZclFoundation: clusterID:%x, attrID:%x, dataType=%x\n", clusterID, attrID, dataType);
    
    if( (clusterID == ZCL_CLUSTER_ID_GEN_ON_OFF) && (attrID == ATTRID_ON_OFF) && (dataType == ZCL_DATATYPE_BOOLEAN) )
    {              
        uint8_t state = zclRspBuff[0];
        zllctrl_on_get_light_on_off(nwkAddr, endpoint, state);
    }
    else if( (clusterID == ZCL_CLUSTER_ID_GEN_LEVEL_CONTROL) && (attrID == ATTRID_LEVEL_CURRENT_LEVEL) && (dataType == ZCL_DATATYPE_UINT8) )
    {    
        uint8_t level = zclRspBuff[0];
        zllctrl_on_get_light_level(nwkAddr, endpoint, level);
    }
    else if( (clusterID == ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL) && (attrID == ATTRID_LIGHTING_COLOR_CONTROL_CURRENT_HUE) && (dataType == ZCL_DATATYPE_UINT8) )
    {
        uint8_t hue = zclRspBuff[0];
        zllctrl_on_get_light_hue(nwkAddr, endpoint, hue);
    }
    else if( (clusterID == ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL) && (attrID == ATTRID_LIGHTING_COLOR_CONTROL_CURRENT_SATURATION) && (dataType == ZCL_DATATYPE_UINT8) )
    {
        uint8_t sat = zclRspBuff[0];
        zllctrl_on_get_light_sat(nwkAddr, endpoint, sat);
    }
    else                
    {
      //unsupported ZCL Read Rsp
      uprintf(UPRINT_WARNING, UPRINT_BLK_HUE, "processRpcSysAppZclFoundation: Unsupported ZCL Rsp\n");
    } 
  }
  else
  {
    //unsupported ZCL Rsp
    uprintf(UPRINT_WARNING, UPRINT_BLK_HUE, "processRpcSysAppZclFoundation: Unsupported ZCL Rsp");
  }
  
  return;                    
}  
 
/*************************************************************************************************
 * @fn      processRpcSysApp()
 *
 * @brief   read and process the RPC App message from the ZLL controller
 *
 * @param   none
 *
 * @return  length of current Rx Buffer
 *************************************************************************************************/
static void processRpcSysApp(uint8_t *rpcBuff)
{
  if( rpcBuff[1] == MT_APP_ZLL_TL_IND )
  {
    processRpcSysAppTlInd(&rpcBuff[2]);
  }          
  else if( rpcBuff[1] == MT_APP_ZLL_NEW_DEV_IND )
  {
    processRpcSysAppNewDevInd(&rpcBuff[2]);
  }  
  else if( rpcBuff[1] == MT_APP_RSP )
  {
    processRpcSysAppZcl(&rpcBuff[2]);
  }    
  else if( rpcBuff[1] == 0 )
  {
    if( rpcBuff[2] == 0)
    {
      uprintf(UPRINT_INFO, UPRINT_BLK_HUE, "processRpcSysApp: Command Received Successfully\n\n");
    }
    else
    {
      uprintf(UPRINT_WARNING, UPRINT_BLK_HUE, "processRpcSysApp: Command Error\n\n");
    }    
  }
  else
  {
    uprintf(UPRINT_WARNING, UPRINT_BLK_HUE, "processRpcSysApp: Unsupported MT App Msg\n");
  }
    
  return;   
}

/*************************************************************************************************
 * @fn      processRpcSysDbg()
 *
 * @brief   read and process the RPC debug message from the ZLL controller
 *
 * @param   none
 *
 * @return  length of current Rx Buffer
 *************************************************************************************************/
static void processRpcSysDbg(uint8_t *rpcBuff)
{
  if( rpcBuff[1] == MT_DEBUG_MSG )
  {
    //we got a debug string
    uprintf(UPRINT_INFO, UPRINT_BLK_HUE, "ZLL debug: %s\n", (char*) &(rpcBuff[2]));
  }              
  else if( rpcBuff[1] == 0 )
  {
    if( rpcBuff[2] == 0)
    {
      uprintf(UPRINT_INFO, UPRINT_BLK_HUE, "processRpcSysDbg: Command Received Successfully\n\n");
    }
    else
    {
      uprintf(UPRINT_WARNING, UPRINT_BLK_HUE, "processRpcSysDbg: Command Error\n\n");
    }    
  }
  else
  {
    uprintf(UPRINT_WARNING, UPRINT_BLK_HUE, "processRpcSysDbg: Unsupported MT App Msg\n");
  }
}

static void processRpcSysZdo(uint8_t *rpcBuff)
{
  uint8_t status;
  uint16_t addr;
  uint8_t startIdx;
  hue_light_t *light;
	
  // rpcBuff[1]: cmd1
  if(rpcBuff[1] == MT_ZDO_END_DEVICE_ANNCE_IND) 
  {
  	addr = *(uint16_t *)&rpcBuff[4];
  	uprintf(UPRINT_INFO, UPRINT_BLK_HUE, "processRpcSysZDO: device announce ind, srcAddr=0x%x nwkAddr=0x%x IEEEAddr=0x%08x%08x\n", 
        *(uint16_t *)&rpcBuff[2], addr, *(uint32_t *)&rpcBuff[6], *(uint32_t *)&rpcBuff[10]);
    // TODO: to fetch this device's info
#if 0
    /* add the new device to light list... */
    light = zllctrl_find_light_by_ieeeaddr(&rpcBuff[6]);
    if(light == NULL)
    {
      light = zllctrl_create_light(epInfo);

      /* TODO: get state of new light */
    }

    if(light != NULL)
    {
      light->reachable = 1;
    }
#endif
  }
  else if(rpcBuff[1] == MT_ZDO_IEEE_ADDR_RSP)
  {
  	status = rpcBuff[2];
	addr = *(uint16_t *)&rpcBuff[11];
	startIdx = rpcBuff[12];
  	uprintf(UPRINT_INFO, UPRINT_BLK_HUE, "processRpcSysZDO: IEEE addr rsp, status=%d nwkAddr=0x%x startIdx=%d\n", status, addr, startIdx);
  }
}

static void processRpcSysSys(uint8_t *rpcBuff)
{
  hue_t *hue = zllctrl_get_hue();
	
  // rpcBuff[1]: cmd1
  if(rpcBuff[1] == MT_SYS_PING) 
  {
    uint16_t Capabilities = *(uint16_t *)&rpcBuff[2];
  	uprintf(UPRINT_INFO, UPRINT_BLK_HUE, "processRpcSysSys: ping successful\n", Capabilities);
  }
  else if(rpcBuff[1] == MT_SYS_VERSION)
  {
  	hue->transport_rev = hue->socbuf[MT_RPC_POS_DAT0];
    hue->product_id = hue->socbuf[MT_RPC_POS_DAT0+1];
    hue->major_rel = hue->socbuf[MT_RPC_POS_DAT0+2];
    hue->minor_rel = hue->socbuf[MT_RPC_POS_DAT0+3];
    hue->maint_rel = hue->socbuf[MT_RPC_POS_DAT0+4];
    uprintf(UPRINT_INFO, UPRINT_BLK_HUE, "trans_rev=%d prod_id=%d version=%d.%d.%d\n",
            hue->transport_rev, hue->product_id,
            hue->major_rel, hue->minor_rel, hue->maint_rel);
  }
  else
  {
    uprintf(UPRINT_WARNING, UPRINT_BLK_HUE, "processRpcSysZDO: unknown cmd1=0x%x\n", rpcBuff[1]);
  }
}

static void processRpcSysUtil(uint8_t *rpcBuff)
{
  hue_t *hue = zllctrl_get_hue();
	
  // rpcBuff[1]: cmd1
  if(rpcBuff[1] == MT_UTIL_GET_DEVICE_INFO)
  {
  	memcpy(hue->ieee_addr, &hue->socbuf[MT_RPC_POS_DAT0+1], 8);
    memcpy(&hue->short_addr, &hue->socbuf[MT_RPC_POS_DAT0+9], 2);
    hue->device_type = hue->socbuf[MT_RPC_POS_DAT0+11];
    hue->device_state = hue->socbuf[MT_RPC_POS_DAT0+12];
    hue->num_assoc_dev = hue->socbuf[MT_RPC_POS_DAT0+13];
    uprintf(UPRINT_INFO, UPRINT_BLK_HUE, "ieee_addr=%08x%08x short_addr=%d dev_type=0x%x dev_state=0x%x num_assoc_dev=%d\n",
            *(uint32_t *)&hue->ieee_addr[0], *(uint32_t *)&hue->ieee_addr[4], hue->short_addr,
            hue->device_type, hue->device_state, hue->num_assoc_dev);
  }
  else
  {
    uprintf(UPRINT_WARNING, UPRINT_BLK_HUE, "processRpcSysUtil: unknown cmd1=0x%x\n", rpcBuff[1]);
  }
}

/*************************************************************************************************
 * @fn      zllSocProcessRpc()
 *
 * @brief   read and process the RPC from the ZLL controller
 *
 * @param   none
 *
 * @return  length of current Rx Buffer
 *************************************************************************************************/
void zllSocProcessRpc (uint8_t *rpcBuff, uint16_t length)
{
  uint8_t rpcLen, sofByte;    

  //read first byte and check it is a SOF
  sofByte = rpcBuff[0];
  rpcLen = rpcBuff[1];
  if ( (sofByte == MT_RPC_SOF) && (length == rpcLen+4))
  {         
      uprintf(UPRINT_INFO, UPRINT_BLK_HUE, "zllSocProcessRpc: Processing CMD0:%x, CMD1:%x\n", rpcBuff[2], rpcBuff[3] );
      //Read CMD0
      switch (rpcBuff[2] & MT_RPC_SUBSYSTEM_MASK) 
      {
        case MT_RPC_SYS_DBG:
        {
          processRpcSysDbg(&rpcBuff[2]);        
          break;       
        }
        case MT_RPC_SYS_APP:
        {
          processRpcSysApp(&rpcBuff[2]);        
          break;       
        }
		case MT_RPC_SYS_ZDO:
        {
		  processRpcSysZdo(&rpcBuff[2]);
		  break;
		}
        case MT_RPC_SYS_SYS:
        {
          processRpcSysSys(&rpcBuff[2]);
		  break;
        }
        case MT_RPC_SYS_UTIL:
        {
          processRpcSysUtil(&rpcBuff[2]);
		  break;
        }
        default:
        {
          uprintf(UPRINT_WARNING, UPRINT_BLK_HUE, "zllSocProcessRpc: CMD0:%x, CMD1:%x, not handled\n", rpcBuff[2], rpcBuff[3] );
          break;
        }
      }
      
  }
  else
  {
      uprintf(UPRINT_WARNING, UPRINT_BLK_HUE, "zllSocProcessRpc: No valid Start Of Frame found, header=0x%08x length=%d\n", 
	  	    *(uint32_t *)rpcBuff, length);
  }
  
  return; 
}

