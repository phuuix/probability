/**
  ******************************************************************************
  * @file    usbh_cdc_core.h
  * @author  MCD Application Team
  * @version V2.1.0
  * @date    19-March-2012
  * @brief   This file contains all the prototypes for the usbh_msc_core.c
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2012 STMicroelectronics</center></h2>
  *
  * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
  * You may not use this file except in compliance with the License.
  * You may obtain a copy of the License at:
  *
  *        http://www.st.com/software_license_agreement_liberty_v2
  *
  * Unless required by applicable law or agreed to in writing, software 
  * distributed under the License is distributed on an "AS IS" BASIS, 
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
  ******************************************************************************
  */ 

/* Define to prevent recursive  ----------------------------------------------*/
#ifndef __USBH_CDC_CORE_H
#define __USBH_CDC_CORE_H

/* Includes ------------------------------------------------------------------*/
#include "usbh_core.h"
#include "usbh_stdreq.h"
#include "usb_bsp.h"
#include "usbh_ioreq.h"
#include "usbh_hcs.h"



/** @addtogroup USBH_LIB
  * @{
  */

/** @addtogroup USBH_CLASS
  * @{
  */

/** @addtogroup USBH_CDC_CLASS
  * @{
  */
  
/** @defgroup USBH_CDC_CORE
  * @brief This file is the Header file for usbh_CDC_core.c
  * @{
  */ 


/** @defgroup USBH_CDC_CORE_Exported_Types
  * @{
  */ 


/* Structure for MSC process */
typedef struct _CDC_Process
{
  uint8_t              state;             // current state
  uint8_t              event;             // current event
  uint8_t              hc_int_in; 
  uint8_t              hc_bulk_in; 
  uint8_t              hc_bulk_out;
  USBH_EpDesc_TypeDef  epInterrupt;
  USBH_EpDesc_TypeDef  epBulkIn;
  USBH_EpDesc_TypeDef  epBulkOut;
  
  uint16_t             RxDataLength;
  uint16_t             RxDataOffset;
  uint8_t              *RxBuffer;
  uint32_t             tPoll;             // poll time
  uint32_t             tLastRx;           // Time of last Rx
  uint16_t             TxDataLength;
  uint16_t             TxDataOffset;
  uint16_t             TxDataPrevOffset;  // initially set to TxDataOffset
  uint8_t              *TxBuffer;
}
CDC_Machine_TypeDef; 


/**
  * @}
  */ 



/** @defgroup USBH_CDC_CORE_Exported_Defines
  * @{
  */
/* state */
#define USBH_CDC_IDLE_STATE                0x00
#define USBH_CDC_DATAIN_STATE              0x01
#define USBH_CDC_DATAOUT_STATE             0x02

/* event */
#define USBH_CDC_EVENT_NULL 0
#define USBH_CDC_EVENT_RXREQ 1
#define USBH_CDC_EVENT_TXREQ 2



#define CDC_COMM_CLASS 0x02
#define CDC_COMM_SUBCLASS 0x02
#define CDC_COMM_PROTOCOL 0x01
#define CDC_DATA_CLASS 0x0A
#define CDC_DATA_SUBCLASS 0x00
#define CDC_DATA_PROTOCOL 0x00

#define USBH_CDC_RX_POLL    0x8 //80ms

/**
  * @}
  */ 

/** @defgroup USBH_CDC_CORE_Exported_Macros
  * @{
  */ 
/**
  * @}
  */ 

/** @defgroup USBH_CDC_CORE_Exported_Variables
  * @{
  */ 
extern USBH_Class_cb_TypeDef  USBH_CDC_cb;
extern CDC_Machine_TypeDef    CDC_Machine;
extern uint8_t MSCErrorCount;

/**
  * @}
  */ 

/** @defgroup USBH_CDC_CORE_Exported_FunctionsPrototype
  * @{
  */ 



/**
  * @}
  */ 

#endif  /* __USBH_CDC_CORE_H */


/**
  * @}
  */ 

/**
  * @}
  */

/**
  * @}
  */ 

/**
  * @}
  */
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/



