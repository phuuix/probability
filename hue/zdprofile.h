/**************************************************************************************************
  Filename:       ZDProfile.h
  Revised:        $Date: 2013-10-02 15:57:50 -0700 (Wed, 02 Oct 2013) $
  Revision:       $Revision: 35529 $

  Description:    This file contains the interface to the Zigbee Device Object.


  Copyright 2004-2013 Texas Instruments Incorporated. All rights reserved.

  IMPORTANT: Your use of this Software is limited to those specific rights
  granted under the terms of a software license agreement between the user
  who downloaded the software, his/her employer (which must be your employer)
  and Texas Instruments Incorporated (the "License").  You may not use this
  Software unless you agree to abide by the terms of the License. The License
  limits your use, and you acknowledge, that the Software may not be modified,
  copied or distributed unless embedded on a Texas Instruments microcontroller
  or used solely and exclusively in conjunction with a Texas Instruments radio
  frequency transceiver, which is integrated into your product.  Other than for
  the foregoing purpose, you may not use, reproduce, copy, prepare derivative
  works of, modify, distribute, perform, display or sell this Software and/or
  its documentation for any purpose.

  YOU FURTHER ACKNOWLEDGE AND AGREE THAT THE SOFTWARE AND DOCUMENTATION ARE
  PROVIDED “AS IS” WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED,
  INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY, TITLE,
  NON-INFRINGEMENT AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL
  TEXAS INSTRUMENTS OR ITS LICENSORS BE LIABLE OR OBLIGATED UNDER CONTRACT,
  NEGLIGENCE, STRICT LIABILITY, CONTRIBUTION, BREACH OF WARRANTY, OR OTHER
  LEGAL EQUITABLE THEORY ANY DIRECT OR INDIRECT DAMAGES OR EXPENSES
  INCLUDING BUT NOT LIMITED TO ANY INCIDENTAL, SPECIAL, INDIRECT, PUNITIVE
  OR CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF PROCUREMENT
  OF SUBSTITUTE GOODS, TECHNOLOGY, SERVICES, OR ANY CLAIMS BY THIRD PARTIES
  (INCLUDING BUT NOT LIMITED TO ANY DEFENSE THEREOF), OR OTHER SIMILAR COSTS.

  Should you have any questions regarding your right to use this Software,
  contact Texas Instruments Incorporated at www.TI.com.
**************************************************************************************************/

#ifndef ZDPROFILE_H
#define ZDPROFILE_H

#ifdef __cplusplus
extern "C"
{
#endif



/*********************************************************************
 * CONSTANTS
 */

#define ZDO_EP 0   // Endpoint of ZDO
#define ZDO_PROFILE_ID            0
#define ZDO_WILDCARD_PROFILE_ID   0xFFFF

// IEEE_addr_req request types
#define ZDP_ADDR_REQTYPE_SINGLE     0
#define ZDP_ADDR_REQTYPE_EXTENDED   1

// ZDO Status Values
#define ZDP_SUCCESS            0x00  // Operation completed successfully
#define ZDP_INVALID_REQTYPE    0x80  // The supplied request type was invalid
#define ZDP_DEVICE_NOT_FOUND   0x81  // Reserved
#define ZDP_INVALID_EP         0x82  // Invalid endpoint value
#define ZDP_NOT_ACTIVE         0x83  // Endpoint not described by a simple desc.
#define ZDP_NOT_SUPPORTED      0x84  // Optional feature not supported
#define ZDP_TIMEOUT            0x85  // Operation has timed out
#define ZDP_NO_MATCH           0x86  // No match for end device bind
#define ZDP_NO_ENTRY           0x88  // Unbind request failed, no entry
#define ZDP_NO_DESCRIPTOR      0x89  // Child descriptor not available
#define ZDP_INSUFFICIENT_SPACE 0x8a  // Insufficient space to support operation
#define ZDP_NOT_PERMITTED      0x8b  // Not in proper state to support operation
#define ZDP_TABLE_FULL         0x8c  // No table space to support operation
#define ZDP_NOT_AUTHORIZED     0x8d  // Permissions indicate request not authorized
#define ZDP_BINDING_TABLE_FULL 0x8e  // No binding table space to support operation

#define ZDP_NETWORK_DISCRIPTOR_SIZE  8
#define ZDP_NETWORK_EXTENDED_DISCRIPTOR_SIZE  14
#define ZDP_RTG_DISCRIPTOR_SIZE      5
#define ZDP_BIND_DISCRIPTOR_SIZE  19

// Mgmt_Permit_Join_req fields
#define ZDP_MGMT_PERMIT_JOIN_REQ_DURATION 0
#define ZDP_MGMT_PERMIT_JOIN_REQ_TC_SIG   1
#define ZDP_MGMT_PERMIT_JOIN_REQ_SIZE     2

// Mgmt_Leave_req fields
#define ZDP_MGMT_LEAVE_REQ_REJOIN      1 << 7
#define ZDP_MGMT_LEAVE_REQ_RC          1 << 6   // Remove Children

// Mgmt_Lqi_rsp - (neighbor table) device type
#define ZDP_MGMT_DT_COORD  0x0
#define ZDP_MGMT_DT_ROUTER 0x1
#define ZDP_MGMT_DT_ENDDEV 0x2

// Mgmt_Lqi_rsp - (neighbor table) relationship
#define ZDP_MGMT_REL_PARENT  0x0
#define ZDP_MGMT_REL_CHILD   0x1
#define ZDP_MGMT_REL_SIBLING 0x2
#define ZDP_MGMT_REL_UNKNOWN 0x3

// Mgmt_Lqi_rsp - (neighbor table) unknown boolean value
#define ZDP_MGMT_BOOL_UNKNOWN 0x02

// Status fields used by ZDO_ProcessMgmtRtgReq
#define ZDO_MGMT_RTG_ENTRY_ACTIVE                 0x00
#define ZDO_MGMT_RTG_ENTRY_DISCOVERY_UNDERWAY     0x01
#define ZDO_MGMT_RTG_ENTRY_DISCOVERY_FAILED       0x02
#define ZDO_MGMT_RTG_ENTRY_INACTIVE               0x03

#define ZDO_MGMT_RTG_ENTRY_MEMORY_CONSTRAINED     0x01
#define ZDO_MGMT_RTG_ENTRY_MANYTOONE              0x02
#define ZDO_MGMT_RTG_ENTRY_ROUTE_RECORD_REQUIRED  0x04

/*********************************************************************
 * Message/Cluster IDS
 */

// ZDO Cluster IDs
#define ZDO_RESPONSE_BIT_V1_0   ((uint8_t)0x80)
#define ZDO_RESPONSE_BIT        ((uint16_t)0x8000)

#define NWK_addr_req            ((uint16_t)0x0000)
#define IEEE_addr_req           ((uint16_t)0x0001)
#define Node_Desc_req           ((uint16_t)0x0002)
#define Power_Desc_req          ((uint16_t)0x0003)
#define Simple_Desc_req         ((uint16_t)0x0004)
#define Active_EP_req           ((uint16_t)0x0005)
#define Match_Desc_req          ((uint16_t)0x0006)
#define NWK_addr_rsp            (NWK_addr_req | ZDO_RESPONSE_BIT)
#define IEEE_addr_rsp           (IEEE_addr_req | ZDO_RESPONSE_BIT)
#define Node_Desc_rsp           (Node_Desc_req | ZDO_RESPONSE_BIT)
#define Power_Desc_rsp          (Power_Desc_req | ZDO_RESPONSE_BIT)
#define Simple_Desc_rsp         (Simple_Desc_req | ZDO_RESPONSE_BIT)
#define Active_EP_rsp           (Active_EP_req | ZDO_RESPONSE_BIT)
#define Match_Desc_rsp          (Match_Desc_req | ZDO_RESPONSE_BIT)

#define Complex_Desc_req        ((uint16_t)0x0010)
#define User_Desc_req           ((uint16_t)0x0011)
#define Discovery_Cache_req     ((uint16_t)0x0012)
#define Device_annce            ((uint16_t)0x0013)
#define User_Desc_set           ((uint16_t)0x0014)
#define Server_Discovery_req    ((uint16_t)0x0015)
#define End_Device_Timeout_req  ((uint16_t)0x001f)
#define Complex_Desc_rsp        (Complex_Desc_req | ZDO_RESPONSE_BIT)
#define User_Desc_rsp           (User_Desc_req | ZDO_RESPONSE_BIT)
#define Discovery_Cache_rsp     (Discovery_Cache_req | ZDO_RESPONSE_BIT)
#define User_Desc_conf          (User_Desc_set | ZDO_RESPONSE_BIT)
#define Server_Discovery_rsp    (Server_Discovery_req | ZDO_RESPONSE_BIT)
#define End_Device_Timeout_rsp  (End_Device_Timeout_req | ZDO_RESPONSE_BIT)

#define End_Device_Bind_req     ((uint16_t)0x0020)
#define Bind_req                ((uint16_t)0x0021)
#define Unbind_req              ((uint16_t)0x0022)
#define Bind_rsp                (Bind_req | ZDO_RESPONSE_BIT)
#define End_Device_Bind_rsp     (End_Device_Bind_req | ZDO_RESPONSE_BIT)
#define Unbind_rsp              (Unbind_req | ZDO_RESPONSE_BIT)

#define Mgmt_NWK_Disc_req       ((uint16_t)0x0030)
#define Mgmt_Lqi_req            ((uint16_t)0x0031)
#define Mgmt_Rtg_req            ((uint16_t)0x0032)
#define Mgmt_Bind_req           ((uint16_t)0x0033)
#define Mgmt_Leave_req          ((uint16_t)0x0034)
#define Mgmt_Direct_Join_req    ((uint16_t)0x0035)
#define Mgmt_Permit_Join_req    ((uint16_t)0x0036)
#define Mgmt_NWK_Update_req     ((uint16_t)0x0038)
#define Mgmt_NWK_Disc_rsp       (Mgmt_NWK_Disc_req | ZDO_RESPONSE_BIT)
#define Mgmt_Lqi_rsp            (Mgmt_Lqi_req | ZDO_RESPONSE_BIT)
#define Mgmt_Rtg_rsp            (Mgmt_Rtg_req | ZDO_RESPONSE_BIT)
#define Mgmt_Bind_rsp           (Mgmt_Bind_req | ZDO_RESPONSE_BIT)
#define Mgmt_Leave_rsp          (Mgmt_Leave_req | ZDO_RESPONSE_BIT)
#define Mgmt_Direct_Join_rsp    (Mgmt_Direct_Join_req | ZDO_RESPONSE_BIT)
#define Mgmt_Permit_Join_rsp    (Mgmt_Permit_Join_req | ZDO_RESPONSE_BIT)
#define Mgmt_NWK_Update_notify  (Mgmt_NWK_Update_req | ZDO_RESPONSE_BIT)

#define ZDO_ALL_MSGS_CLUSTERID  0xFFFF



/*********************************************************************
*********************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* ZDPROFILE_H */
