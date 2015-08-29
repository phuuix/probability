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
void zllSocClose( void );
void zllSocProcessRpc (uint8_t *rpcBuff, uint16_t length);

//ZLL API's
uint32_t zllSocTouchLink(uint8_t *cmbbuf);
uint32_t zllSocResetToFn(uint8_t *cmbbuf);
uint32_t zllSocSendResetToFn(uint8_t *cmbbuf);
uint32_t zllSocOpenNwk(uint8_t *cmbbuf, uint8_t duration);
//ZCL Set API's
uint32_t zllSocSetState(uint8_t *cmbbuf, uint8_t state, uint16_t dstAddr, uint8_t endpoint, uint8_t addrMode);
uint32_t zllSocSetLevel(uint8_t *cmbbuf, uint8_t level, uint16_t time, uint16_t dstAddr, uint8_t endpoint, uint8_t addrMode);
uint32_t zllSocSetHue(uint8_t *cmbbuf, uint8_t hue, uint16_t time, uint16_t dstAddr, uint8_t endpoint, uint8_t addrMode);
uint32_t zllSocSetSat(uint8_t *cmbbuf, uint8_t sat, uint16_t time, uint16_t dstAddr, uint8_t  endpoint, uint8_t addrMode);
uint32_t zllSocSetColor(uint8_t *cmbbuf, uint16_t x, uint16_t y, uint16_t time, uint16_t dstAddr, uint8_t endpoint, uint8_t addrMode);
uint32_t zllSocSetHueSat(uint8_t *cmbbuf, uint8_t hue, uint8_t sat, uint16_t time, uint16_t dstAddr, uint8_t endpoint, uint8_t addrMode);
uint32_t zllSocAddGroup(uint8_t *cmbbuf, uint16_t groupId, uint16_t dstAddr, uint8_t endpoint, uint8_t addrMode);
uint32_t zllSocStoreScene(uint8_t *cmbbuf, uint16_t groupId, uint8_t sceneId, uint16_t dstAddr, uint8_t endpoint, uint8_t addrMode);
uint32_t zllSocRecallScene(uint8_t *cmbbuf, uint16_t groupId, uint8_t sceneId, uint16_t dstAddr, uint8_t endpoint, uint8_t addrMode);
uint32_t zllSocBind(uint8_t *cmbbuf, uint16_t srcNwkAddr, uint8_t srcEndpoint, uint8_t srcIEEE[8], uint8_t dstEndpoint, uint8_t dstIEEE[8], uint16_t clusterID);
//ZCL Get API's
uint32_t zllSocGetState(uint8_t *cmbbuf, uint16_t dstAddr, uint8_t endpoint, uint8_t addrMode);
uint32_t zllSocGetLevel(uint8_t *cmbbuf, uint16_t dstAddr, uint8_t endpoint, uint8_t addrMode);
uint32_t zllSocGetHue(uint8_t *cmbbuf, uint16_t dstAddr, uint8_t endpoint, uint8_t addrMode);
uint32_t zllSocGetSat(uint8_t *cmbbuf, uint16_t dstAddr, uint8_t endpoint, uint8_t addrMode);
uint32_t zllSocSysPing(uint8_t *cmbbuf);
uint32_t zllSocSysVersion(uint8_t *cmbbuf);
uint32_t zllSocUtilGetDevInfo(uint8_t *cmbbuf);

#ifdef __cplusplus
}
#endif

#endif /* ZLLSOCCMD_H */
