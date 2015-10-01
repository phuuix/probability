/**
  ******************************************************************************
  * @file    usbh_usr.c
  * @author  MCD Application Team
  * @version V2.1.0
  * @date    19-March-2012
  * @brief   This file includes the usb host library user callbacks
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

/* puhuix 2014-12-20: modify this file for TI Zigbee dongle cc2531 */
/* Includes ------------------------------------------------------------------*/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "usbh_usr.h"
#include "usbh_cdc_core.h"
#include "usb_hcd_int.h"

#include "task.h"
#include "uprintf.h"
#include "ipc.h"
/** @addtogroup USBH_USER
* @{
*/

/** @addtogroup USBH_MSC_DEMO_USER_CALLBACKS
* @{
*/

/** @defgroup USBH_USR 
* @brief    This file includes the usb host stack user callbacks
* @{
*/ 

/** @defgroup USBH_USR_Private_TypesDefinitions
* @{
*/ 
/**
* @}
*/ 


/** @defgroup USBH_USR_Private_Defines
* @{
*/ 

/**
* @}
*/ 


/** @defgroup USBH_USR_Private_Macros
* @{
*/ 
__ALIGN_BEGIN USB_OTG_CORE_HANDLE	   USB_OTG_Core __ALIGN_END;
__ALIGN_BEGIN USBH_HOST                USB_Host __ALIGN_END;

/**
* @}
*/ 


/** @defgroup USBH_USR_Private_Variables
* @{
*/ 
uint8_t USBH_USR_ApplicationState = USH_USR_FS_INIT;


/*  Points to the DEVICE_PROP structure of current device */
/*  The purpose of this register is to speed up the execution */

USBH_Usr_cb_TypeDef USR_cb =
{
  USBH_USR_Init,
  USBH_USR_DeInit,
  USBH_USR_DeviceAttached,
  USBH_USR_ResetDevice,
  USBH_USR_DeviceDisconnected,
  USBH_USR_OverCurrentDetected,
  USBH_USR_DeviceSpeedDetected,
  USBH_USR_Device_DescAvailable,
  USBH_USR_DeviceAddressAssigned,
  USBH_USR_Configuration_DescAvailable,
  USBH_USR_Manufacturer_String,
  USBH_USR_Product_String,
  USBH_USR_SerialNum_String,
  USBH_USR_EnumerationDone,
  USBH_USR_UserInput,
  USBH_USR_CDC_Application,
  USBH_USR_DeviceNotSupported,
  USBH_USR_UnrecoveredError
    
};

/**
* @}
*/

/** @defgroup USBH_USR_Private_Constants
* @{
*/ 
/*--------------- LCD Messages ---------------*/
const uint8_t MSG_HOST_INIT[]        = "> Host Library Initialized\n";
const uint8_t MSG_DEV_ATTACHED[]     = "> Device Attached \n";
const uint8_t MSG_DEV_DISCONNECTED[] = "> Device Disconnected\n";
const uint8_t MSG_DEV_ENUMERATED[]   = "> Enumeration completed \n";
const uint8_t MSG_DEV_HIGHSPEED[]    = "> High speed device detected\n";
const uint8_t MSG_DEV_FULLSPEED[]    = "> Full speed device detected\n";
const uint8_t MSG_DEV_LOWSPEED[]     = "> Low speed device detected\n";
const uint8_t MSG_DEV_ERROR[]        = "> Device fault \n";
const uint8_t MSG_UNREC_ERROR[]      = "> UNRECOVERED ERROR STATE\n";

/**
* @}
*/


/** @defgroup USBH_USR_Private_FunctionPrototypes
* @{
*/

/**
* @}
*/ 


/** @defgroup USBH_USR_Private_Functions
* @{
*/ 


/**
* @brief  USBH_USR_Init 
*         Displays the message on LCD for host lib initialization
* @param  None
* @retval None
*/
void USBH_USR_Init(void)
{
  static uint8_t startup = 0;  
  
  if(startup == 0 )
  {
    startup = 1;
      
#ifdef USE_USB_OTG_HS 
    uprintf(UPRINT_INFO, UPRINT_BLK_DEF, " USB OTG HS MSC Host");
#else
    uprintf(UPRINT_INFO, UPRINT_BLK_DEF, " USB OTG FS MSC Host");
#endif
    uprintf(UPRINT_INFO, UPRINT_BLK_DEF, "> USB Host library started.\n"); 
    uprintf(UPRINT_INFO, UPRINT_BLK_DEF, "     USB Host Library v2.1.0\n" );
  }
}

/**
* @brief  USBH_USR_DeviceAttached 
*         Displays the message on LCD on device attached
* @param  None
* @retval None
*/
void USBH_USR_DeviceAttached(void)
{
  uprintf(UPRINT_INFO, UPRINT_BLK_DEF,(void *)MSG_DEV_ATTACHED);
}


/**
* @brief  USBH_USR_UnrecoveredError
* @param  None
* @retval None
*/
void USBH_USR_UnrecoveredError (void)
{
  
  /* Set default screen color*/ 
  uprintf(UPRINT_INFO, UPRINT_BLK_DEF,(void *)MSG_UNREC_ERROR); 
}


/**
* @brief  USBH_DisconnectEvent
*         Device disconnect event
* @param  None
* @retval Staus
*/
void USBH_USR_DeviceDisconnected (void)
{
  /* Set default screen color*/
  uprintf(UPRINT_INFO, UPRINT_BLK_DEF,(void *)MSG_DEV_DISCONNECTED);
}
/**
* @brief  USBH_USR_ResetUSBDevice 
* @param  None
* @retval None
*/
void USBH_USR_ResetDevice(void)
{
  /* callback for USB-Reset */
  uprintf(UPRINT_INFO, UPRINT_BLK_DEF,(void *)"Device reset\n");
}


/**
* @brief  USBH_USR_DeviceSpeedDetected 
*         Displays the message on LCD for device speed
* @param  Device speed
* @retval None
*/
void USBH_USR_DeviceSpeedDetected(uint8_t DeviceSpeed)
{
  if(DeviceSpeed == HPRT0_PRTSPD_HIGH_SPEED)
  {
    uprintf(UPRINT_INFO, UPRINT_BLK_DEF,(void *)MSG_DEV_HIGHSPEED);
  }  
  else if(DeviceSpeed == HPRT0_PRTSPD_FULL_SPEED)
  {
    uprintf(UPRINT_INFO, UPRINT_BLK_DEF,(void *)MSG_DEV_FULLSPEED);
  }
  else if(DeviceSpeed == HPRT0_PRTSPD_LOW_SPEED)
  {
    uprintf(UPRINT_INFO, UPRINT_BLK_DEF,(void *)MSG_DEV_LOWSPEED);
  }
  else
  {
    uprintf(UPRINT_INFO, UPRINT_BLK_DEF,(void *)MSG_DEV_ERROR);
  }
}

/**
* @brief  USBH_USR_Device_DescAvailable 
*         Displays the message on LCD for device descriptor
* @param  device descriptor
* @retval None
*/
void USBH_USR_Device_DescAvailable(void *DeviceDesc)
{ 
  USBH_DevDesc_TypeDef *hs;
  hs = DeviceDesc;  
  
  
  uprintf(UPRINT_INFO, UPRINT_BLK_DEF,"VID : %04Xh\n" , (uint32_t)(*hs).idVendor); 
  uprintf(UPRINT_INFO, UPRINT_BLK_DEF,"PID : %04Xh\n" , (uint32_t)(*hs).idProduct); 
}

/**
* @brief  USBH_USR_DeviceAddressAssigned 
*         USB device is successfully assigned the Address 
* @param  None
* @retval None
*/
void USBH_USR_DeviceAddressAssigned(void)
{
  
}


/**
* @brief  USBH_USR_Conf_Desc 
*         Displays the message on LCD for configuration descriptor
* @param  Configuration descriptor
* @retval None
*/
void USBH_USR_Configuration_DescAvailable(USBH_CfgDesc_TypeDef * cfgDesc,
                                          USBH_InterfaceDesc_TypeDef *itfDesc,
                                          USBH_EpDesc_TypeDef *epDesc)
{
  uint8_t i;  

  for(i=0; i<USBH_MAX_NUM_INTERFACES; i++)
  {
	if(itfDesc[i].bInterfaceClass  == 0x02)
	{
		uprintf(UPRINT_INFO, UPRINT_BLK_DEF, "CDC Interrupt Class\n");
	}
	else if(itfDesc[i].bInterfaceClass  == 0x0A)
	{
		uprintf(UPRINT_INFO, UPRINT_BLK_DEF, "CDC Data Class\n");
	}
  }
}

/**
* @brief  USBH_USR_Manufacturer_String 
*         Displays the message on LCD for Manufacturer String 
* @param  Manufacturer String 
* @retval None
*/
void USBH_USR_Manufacturer_String(void *ManufacturerString)
{
  uprintf(UPRINT_INFO, UPRINT_BLK_DEF, "Manufacturer : %s\n", (char *)ManufacturerString);
}

/**
* @brief  USBH_USR_Product_String 
*         Displays the message on LCD for Product String
* @param  Product String
* @retval None
*/
void USBH_USR_Product_String(void *ProductString)
{
  uprintf(UPRINT_INFO, UPRINT_BLK_DEF, "Product : %s\n", (char *)ProductString);  
}

/**
* @brief  USBH_USR_SerialNum_String 
*         Displays the message on LCD for SerialNum_String 
* @param  SerialNum_String 
* @retval None
*/
void USBH_USR_SerialNum_String(void *SerialNumString)
{
  uprintf(UPRINT_INFO, UPRINT_BLK_DEF, "Serial Number : %s\n", (char *)SerialNumString);    
} 



/**
* @brief  EnumerationDone 
*         User response request is displayed to ask application jump to class
* @param  None
* @retval None
*/
void USBH_USR_EnumerationDone(void)
{
  
  /* Enumeration complete */
  uprintf(UPRINT_INFO, UPRINT_BLK_DEF, (char *)MSG_DEV_ENUMERATED);
  
} 


/**
* @brief  USBH_USR_DeviceNotSupported
*         Device is not supported
* @param  None
* @retval None
*/
void USBH_USR_DeviceNotSupported(void)
{
  uprintf(UPRINT_INFO, UPRINT_BLK_DEF, "> Device not supported."); 
}  


/**
* @brief  USBH_USR_UserInput
*         User Action for application state entry
* @param  None
* @retval USBH_USR_Status : User response for key button
*/
USBH_USR_Status USBH_USR_UserInput(void)
{
  USBH_USR_Status usbh_usr_status;
  
  usbh_usr_status = USBH_USR_RESP_OK;
    
  return usbh_usr_status;
}  

/**
* @brief  USBH_USR_OverCurrentDetected
*         Over Current Detected on VBUS
* @param  None
* @retval Staus
*/
void USBH_USR_OverCurrentDetected (void)
{
  uprintf(UPRINT_INFO, UPRINT_BLK_DEF, "Overcurrent detected.");
}


/**
* @brief  USBH_USR_CDC_Application 
*         Demo application for mass storage
* @param  None
* @retval Staus
*/
int USBH_USR_CDC_Application(void)
{
  return(0);
}


/**
* @brief  USBH_USR_DeInit
*         Deint User state and associated variables
* @param  None
* @retval None
*/
void USBH_USR_DeInit(void)
{
  USBH_USR_ApplicationState = USH_USR_FS_INIT;
}


/**
* @}
*/ 

#define CDC_MSG_TYPE_TXDONE 0
#define CDC_MSG_TYPE_RXDONE 1
#define CDC_MSG_TYPE_TXREQ 2
#define CDC_MSG_TYPE_RXREQ 3

struct mailbox cdc_mbox;

// TODO: refine below code
uint8_t usbh_usr_cdc_rxbuf[256];
uint8_t usbh_usr_cdc_txbuf[256];
extern int zllctrl_post_zll_message(uint8_t *message, uint16_t length);
extern void zllSocProcessRpc(uint8_t *rpcBuff, uint16_t length);
extern int zllctrl_init();
extern void usbh_cdc_set_event(CDC_Machine_TypeDef *pCDC_Machine, uint8_t event);

/* requested data is received successfully
 *   RxBuffer: equals to pCDC_Machine->RxBuffer
 *   RxDataLength: equals to pCDC_Machine->RxDataLength and RxDataOffset
 */
void usbh_usr_cdc_rxdone(CDC_Machine_TypeDef *pCDC_Machine, uint8_t *RxBuffer, uint8_t RxDataLength)
{
	uint8_t length;
    uint32_t *pData = (uint32_t *)RxBuffer;

	length = RxBuffer[1]+5; /* 5: SOF + LEN + CMD0 + CMD1 + FCS */
    uprintf(UPRINT_INFO, UPRINT_BLK_DEF, "CDC Rx done: length=%d data: 0x%08x %08x %08x %08x\n", 
        length, pData[0], pData[4], pData[8], pData[12]);
    //kprintf("CDC Rx done: length=%d RxDataLength=%d\n", length, RxDataLength);
    //dump_buffer(pCDC_Machine->RxBuffer, pCDC_Machine->RxBuffer[1]+4);

	if(RxDataLength < length)
	{
		/* receive the reminder data and store them into the same buffer: 
		 *   keep  RxDataOffset untouched and increase RxDataLength to total length
		 */
		pCDC_Machine->RxDataLength = length;
		usbh_cdc_set_event(pCDC_Machine, USBH_CDC_EVENT_RXREQ);
	}
	else
	{
		/* Rx real done, notify high layer */
        zllctrl_post_zll_message(pCDC_Machine->RxBuffer, RxBuffer[1]+4);
		//zllSocProcessRpc(pCDC_Machine->RxBuffer, RxBuffer[1]+4);
		pCDC_Machine->RxDataLength = 0;
		pCDC_Machine->RxDataOffset = 0;
		*(uint32_t *)pCDC_Machine->RxBuffer = 0;
	}
}

/* start to Rx message */
int usbh_usr_cdc_rxreq(uint8_t *buffer, uint8_t length)
{	
	/* start to Rx */	
	CDC_Machine.RxDataLength = length;
	CDC_Machine.RxDataOffset = 0;
	CDC_Machine.RxBuffer = buffer; // always usbh_usr_cdc_rxbuf
	usbh_cdc_set_event(&CDC_Machine, USBH_CDC_EVENT_RXREQ);

	return 0;
}

/* notify cdc task tx is done.
  */
void usbh_usr_cdc_txdone(CDC_Machine_TypeDef *pCDC_Machine, uint8_t *TxBuffer, uint8_t TxDataLength)
{
    uint32_t *pData = (uint32_t *)TxBuffer;
    // debug code
    uprintf(UPRINT_INFO, UPRINT_BLK_DEF, "CDC Tx done: length=%d data: 0x%08x %08x %08x %08x\n", 
        pCDC_Machine->TxDataLength, pData[0], pData[4], pData[8], pData[12]);
    //kprintf("CDC Tx done: length=%d\n", pCDC_Machine->TxDataLength);
    //dump_buffer(TxBuffer, TxDataLength);

	/* start Rx immediately */
	usbh_usr_cdc_rxreq(usbh_usr_cdc_rxbuf, pCDC_Machine->epBulkIn.wMaxPacketSize);
}

int usbh_usr_cdc_isTxIdle()
{
    return (CDC_Machine.TxDataLength==0);
}

/* start to Tx message */
int usbh_usr_cdc_txreq(uint8_t *buff, uint8_t length)
{
	if(usbh_usr_cdc_isTxIdle())
	{
		memcpy(usbh_usr_cdc_txbuf, buff, length);
		CDC_Machine.TxBuffer = usbh_usr_cdc_txbuf;
		CDC_Machine.TxDataOffset = 0;
		CDC_Machine.TxDataPrevOffset = 0;
		CDC_Machine.TxDataLength = length;
		usbh_cdc_set_event(&CDC_Machine, USBH_CDC_EVENT_TXREQ);
		
		return length;
	}
	
	return 0;
}

/**
* @}
*/ 
/**
  * @brief  OTG_FS_IRQHandler
  *          This function handles USB-On-The-Go FS global interrupt request.
  *          requests.
  * @param  None
  * @retval None
  */
#ifdef USE_USB_OTG_FS  
void OTG_FS_IRQHandler(void)
#else
void OTG_HS_IRQHandler(void)
#endif
{
  USBH_OTG_ISR_Handler(&USB_OTG_Core);
}

/**
* @}
*/
void cdc_mail_handle(uint32_t *mail)
{
	uint8_t type;
	uint8_t length, txlen = 0;
	uint8_t *message;

	type = (mail[0] & 0xFF000000)>>24;
	
	switch(type)
	{
		case CDC_MSG_TYPE_TXREQ:
			/* start transfer */
			length = (mail[0] & 0x00FF0000)>>16;
			message = (uint8_t *)mail[1];
			txlen = usbh_usr_cdc_txreq(message, length);
            // CDC is busy but we can't wait as Tx done is in same loop
            // To fix this busy issue, prompt the priority of CDC task; or use ping-pong buffer...
            if(txlen != length)
                uprintf(UPRINT_WARNING, UPRINT_BLK_DEF, "Warning: CDC Tx failed: length=%d\n", length);
            free(message);
			break;

		case CDC_MSG_TYPE_RXREQ:
			/* start Rx immediately */
			usbh_usr_cdc_rxreq(usbh_usr_cdc_rxbuf, CDC_Machine.epBulkIn.wMaxPacketSize);
			break;

		default:
			uprintf(UPRINT_WARNING, UPRINT_BLK_DEF, "unknown cdc command type: %d\n", type);
			break;
	}
}
/**
  * adapt to OS
  * @}
  */ 
void usbh_cdc_task(void *p)
{
	uint32_t mail[2];
	int ret;
	
	assert(ROK == mbox_initialize(&cdc_mbox, 2, 16, NULL));
	
	/* Init Host Stack */
	USBH_Init(&USB_OTG_Core, USB_OTG_FS_CORE_ID, &USB_Host, &USBH_CDC_cb, &USR_cb);
	
	zllctrl_init();
	
	for(;;)
	{
        if(usbh_usr_cdc_isTxIdle())
        {
    		ret = mbox_pend(&cdc_mbox, mail, 1);
    		switch (ret)
    		{
    			case ROK:
    				/* handle command: rx, tx ... */
    				cdc_mail_handle(mail);
    				break;
    			case RTIMEOUT:
    				break;
    			default:
    				break;
    		}
        }
        else
            task_delay(1);
		
		/* Host Task handler */
		USBH_Process(&USB_OTG_Core, &USB_Host);
	}
}

void usbh_cdc_init()
{
	task_t t;
	/* create tcdc task */
	t = task_create("tcdc", usbh_cdc_task, NULL, NULL, /* stack size */ 0x400, /* priority */ 4, 20, 0);
	assert(t > 0);
	task_resume_noschedule(t);
}

/**
* @}
*/

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

