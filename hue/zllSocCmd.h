/*
 * zllSocCmd.h
 *
 * This module contains the API for the zll SoC Host Interface.
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

#ifndef ZLLSOCCMD_H
#define ZLLSOCCMD_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>

/********************************************************************/
// ZLL Soc Types

// Endpoint information record entry
typedef struct
{
  uint8_t IEEEAddr[8];
  uint16_t nwkAddr;   // Network address
  uint8_t endpoint;   // Endpoint identifier
  uint16_t profileID; // Profile identifier
  uint16_t deviceID;  // Device identifier
  uint8_t version;    // Version
  char* deviceName;
  uint8_t status;
} epInfo_t;

typedef uint8_t (*zllSocTlIndicationCb_t)(epInfo_t *epInfo);
typedef uint8_t (*zllNewDevIndicationCb_t)(epInfo_t *epInfo);
typedef uint8_t (*zllSocZclGetStateCb_t)(uint8_t state, uint16_t nwkAddr, uint8_t endpoint);
typedef uint8_t (*zllSocZclGetLevelCb_t)(uint8_t level, uint16_t nwkAddr, uint8_t endpoint);
typedef uint8_t (*zllSocZclGetHueCb_t)(uint8_t hue, uint16_t nwkAddr, uint8_t endpoint);
typedef uint8_t (*zllSocZclGetSatCb_t)(uint8_t sat, uint16_t nwkAddr, uint8_t endpoint);

typedef struct
{
  zllSocTlIndicationCb_t          pfnTlIndicationCb;      // TouchLink Indication callback
  zllNewDevIndicationCb_t         pfnNewDevIndicationCb;  // New device Indication callback    
  zllSocZclGetStateCb_t           pfnZclGetStateCb;       // ZCL response callback for get State
  zllSocZclGetLevelCb_t           pfnZclGetLevelCb;     // ZCL response callback for get Level
  zllSocZclGetHueCb_t             pfnZclGetHueCb;         // ZCL response callback for get Hue
  zllSocZclGetSatCb_t             pfnZclGetSatCb;         // ZCL response callback for get Sat
} zllSocCallbacks_t;

#define Z_EXTADDR_LEN 8

typedef enum
{
  afAddrNotPresent = 0,
  afAddrGroup      = 1,
  afAddr16Bit      = 2,
  afAddr64Bit      = 3,  
  afAddrBroadcast  = 15
} afAddrMode_t;

/********************************************************************/
// ZLL Soc API

//configuration API's
int32_t zllSocOpen( char *devicePath );
void zllSocRegisterCallbacks( zllSocCallbacks_t zllSocCallbacks);
void zllSocClose( void );
void zllSocProcessRpc (uint8_t *rpcBuff, uint16_t length);

//ZLL API's
void zllSocTouchLink(void);
void zllSocResetToFn(void);
void zllSocSendResetToFn(void);
void zllSocOpenNwk(uint8_t duration);
//ZCL Set API's
void zllSocSetState(uint8_t state, uint16_t dstAddr, uint8_t endpoint, uint8_t addrMode);
void zllSocSetLevel(uint8_t level, uint16_t time, uint16_t dstAddr, uint8_t endpoint, uint8_t addrMode);
void zllSocSetHue(uint8_t hue, uint16_t time, uint16_t dstAddr, uint8_t endpoint, uint8_t addrMode);
void zllSocSetSat(uint8_t sat, uint16_t time, uint16_t dstAddr, uint8_t  endpoint, uint8_t addrMode);
void zllSocSetHueSat(uint8_t hue, uint8_t sat, uint16_t time, uint16_t dstAddr, uint8_t endpoint, uint8_t addrMode);
void zllSocAddGroup(uint16_t groupId, uint16_t dstAddr, uint8_t endpoint, uint8_t addrMode);
void zllSocStoreScene(uint16_t groupId, uint8_t sceneId, uint16_t dstAddr, uint8_t endpoint, uint8_t addrMode);
void zllSocRecallScene(uint16_t groupId, uint8_t sceneId, uint16_t dstAddr, uint8_t endpoint, uint8_t addrMode);
void zllSocBind(uint16_t srcNwkAddr, uint8_t srcEndpoint, uint8_t srcIEEE[8], uint8_t dstEndpoint, uint8_t dstIEEE[8], uint16_t clusterID);
//ZCL Get API's
void zllSocGetState(uint16_t dstAddr, uint8_t endpoint, uint8_t addrMode);
void zllSocGetLevel(uint16_t dstAddr, uint8_t endpoint, uint8_t addrMode);
void zllSocGetHue(uint16_t dstAddr, uint8_t endpoint, uint8_t addrMode);
void zllSocGetSat(uint16_t dstAddr, uint8_t endpoint, uint8_t addrMode);
void zllSocSysPing();
#ifdef __cplusplus
}
#endif

#endif /* ZLLSOCCMD_H */
