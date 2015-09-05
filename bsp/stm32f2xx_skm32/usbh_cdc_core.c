/**
  ******************************************************************************
  * @file    usbh_cdc_core.c
  * @author  MCD Application Team
  * @version V2.1.0
  * @date    19-March-2012
  * @brief   This file implements the CDC class driver functions
  *      
  *  @endverbatim
  *
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

#include "usbh_cdc_core.h"
#include "usbh_core.h"
#include "usb_core.h"

#include "uprintf.h"
#include "clock.h"
#include <string.h>
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
  * @brief    This file includes the mass storage related functions
  * @{
  */ 


/** @defgroup USBH_CDC_CORE_Private_TypesDefinitions
  * @{
  */ 
/**
  * @}
  */ 

/** @defgroup USBH_CDC_CORE_Private_Defines
  * @{
  */ 
#define USBH_CDC_ERROR_RETRY_LIMIT 10
/**
  * @}
  */ 

/** @defgroup USBH_CDC_CORE_Private_Macros
  * @{
  */ 
/**
  * @}
  */ 


/** @defgroup USBH_CDC_CORE_Private_Variables
  * @{
  */ 
#ifdef USB_OTG_HS_INTERNAL_DMA_ENABLED
  #if defined ( __ICCARM__ ) /*!< IAR Compiler */
    #pragma data_alignment=4   
  #endif
#endif /* USB_OTG_HS_INTERNAL_DMA_ENABLED */
__ALIGN_BEGIN CDC_Machine_TypeDef         CDC_Machine __ALIGN_END ;

/**
  * @}
  */ 


/** @defgroup USBH_CDC_CORE_Private_FunctionPrototypes
  * @{
  */ 

static USBH_Status USBH_CDC_InterfaceInit  (USB_OTG_CORE_HANDLE *pdev , 
                                            void *phost);

static void USBH_CDC_InterfaceDeInit  (USB_OTG_CORE_HANDLE *pdev , 
                                       void *phost);

static USBH_Status USBH_CDC_Handle(USB_OTG_CORE_HANDLE *pdev , 
                            void *phost);

static USBH_Status USBH_CDC_ClassRequest(USB_OTG_CORE_HANDLE *pdev , 
                                         void *phost);

USBH_Class_cb_TypeDef  USBH_CDC_cb = 
{
  USBH_CDC_InterfaceInit,
  USBH_CDC_InterfaceDeInit,
  USBH_CDC_ClassRequest,
  USBH_CDC_Handle,
};


/**
  * @}
  */ 

extern USBH_Usr_cb_TypeDef    USR_cb;

extern uint8_t usbh_usr_cdc_rxdone(CDC_Machine_TypeDef *pCDC_Machine, uint8_t *RxBuffer, uint8_t RxDataLength);
extern uint8_t usbh_usr_cdc_txdone(CDC_Machine_TypeDef *pCDC_Machine, uint8_t *TxBuffer, uint8_t TxDataLength);
/** @defgroup USBH_CDC_CORE_Exported_Variables
  * @{
  */ 

/**
  * @}
  */ 


/** @defgroup USBH_CDC_CORE_Private_Functions
  * @{
  */ 



/**
  * @brief  USBH_CDC_InterfaceInit 
  *         Interface initialization for MSC class.
  * @param  pdev: Selected device
  * @param  hdev: Selected device property
  * @retval USBH_Status : Status of class request handled.
  */
static USBH_Status USBH_CDC_InterfaceInit ( USB_OTG_CORE_HANDLE *pdev, 
                                        void *phost)
{	 
  USBH_HOST *pphost = phost;
  uint16_t i, e;
  
  memset(&CDC_Machine, 0, sizeof(CDC_Machine));

  for(i=0; i<USBH_MAX_NUM_INTERFACES; i++)
  {
	  /* 
	a) Check the interface triad (class, subclass, protocol) on the interface descriptors.
	If you would support ANY CDC-ACM device, this one should be the way.

	CDC-ACM has two interfaces (IFs),
	- Communication Class IF (0x02, 0x02, 0x01)
	- Data Class IF (0x0A, 0x00, 0x00)
	You should check both of interfaces.
	  */
	  if(((pphost->device_prop.Itf_Desc[i].bInterfaceClass == CDC_COMM_CLASS) && 
	     (pphost->device_prop.Itf_Desc[i].bInterfaceSubClass == CDC_COMM_SUBCLASS) && 
	     (pphost->device_prop.Itf_Desc[i].bInterfaceProtocol == CDC_COMM_PROTOCOL)) ||
		 ((pphost->device_prop.Itf_Desc[i].bInterfaceClass == CDC_DATA_CLASS) && 
	     (pphost->device_prop.Itf_Desc[i].bInterfaceSubClass == CDC_DATA_SUBCLASS) && 
	     (pphost->device_prop.Itf_Desc[i].bInterfaceProtocol == CDC_DATA_PROTOCOL)))
	  {
	  /* 
	b) As the next step, identifies the address of each endpoint.
	CDC-ACM has three endpoints,
	Communication Class IF
	- interrupt IN
	Data Class IF
	- bulk IN
	- bulk OUT

	The first endpoint descriptor on the first interface (usually, it's interrupt IN) is referred as,
	pphost->device_prop.Ep_Desc[0][0].bEndpointAddress (IN:bit7=1, OUT:bit7=0)
	pphost->device_prop.Ep_Desc[0][0].bmAttributes (interrupt:0x03, bulk:0x02)
	pphost->device_prop.Ep_Desc[0][0].wMaxPacketSize
	  */
	    for(e=0; e<pphost->device_prop.Itf_Desc[i].bNumEndpoints; e++)
		{
			if(pphost->device_prop.Ep_Desc[i][e].wMaxPacketSize == 0)
				continue;
			
			if(pphost->device_prop.Ep_Desc[i][e].bEndpointAddress & 0x80)
			{
				if(pphost->device_prop.Ep_Desc[i][e].bmAttributes == 0x03)
				{
					CDC_Machine.epInterrupt = pphost->device_prop.Ep_Desc[i][e];
				}
				else if(pphost->device_prop.Ep_Desc[i][e].bmAttributes == 0x02)
				{
					CDC_Machine.epBulkIn = pphost->device_prop.Ep_Desc[i][e];
				}
			}
			else
			{
				if(pphost->device_prop.Ep_Desc[i][e].bmAttributes == 0x02)
				{
					CDC_Machine.epBulkOut = pphost->device_prop.Ep_Desc[i][e];
				}
			}
		}
	  }
  }
  
  if(CDC_Machine.epInterrupt.wMaxPacketSize && CDC_Machine.epBulkIn.wMaxPacketSize && CDC_Machine.epBulkOut.wMaxPacketSize)
  {
	/* assign channel and open channel */
    CDC_Machine.hc_bulk_out = USBH_Alloc_Channel(pdev, CDC_Machine.epBulkOut.bEndpointAddress);
    CDC_Machine.hc_bulk_in = USBH_Alloc_Channel(pdev, CDC_Machine.epBulkIn.bEndpointAddress);  
	CDC_Machine.hc_int_in = USBH_Alloc_Channel(pdev, CDC_Machine.epInterrupt.bEndpointAddress);
    
    /* Open the new channels */
    USBH_Open_Channel  (pdev,
                        CDC_Machine.hc_bulk_out,
                        pphost->device_prop.address,
                        pphost->device_prop.speed,
                        EP_TYPE_BULK,
                        CDC_Machine.epInterrupt.wMaxPacketSize);  
    
    USBH_Open_Channel  (pdev,
                        CDC_Machine.hc_bulk_in,
                        pphost->device_prop.address,
                        pphost->device_prop.speed,
                        EP_TYPE_BULK,
                        CDC_Machine.epBulkIn.wMaxPacketSize);
						
	USBH_Open_Channel  (pdev,
                        CDC_Machine.hc_int_in,
                        pphost->device_prop.address,
                        pphost->device_prop.speed,
                        EP_TYPE_INTR,
                        CDC_Machine.epBulkOut.wMaxPacketSize);

	uprintf_default("EP int: hc=%d addr=0x%x epSize=%d interval=%d\n", 
		             CDC_Machine.hc_int_in, CDC_Machine.epInterrupt.bEndpointAddress, 
		             CDC_Machine.epInterrupt.wMaxPacketSize, CDC_Machine.epInterrupt.bInterval);
	uprintf_default("EP bulk in: hc=%d addr=0x%x epSize=%d interval=%d\n", 
		             CDC_Machine.hc_bulk_in, CDC_Machine.epBulkIn.bEndpointAddress, 
		             CDC_Machine.epBulkIn.wMaxPacketSize, CDC_Machine.epBulkIn.bInterval);
	uprintf_default("EP bulk out: hc=%d addr=0x%x epSize=%d interval=%d\n", 
		             CDC_Machine.hc_bulk_out, CDC_Machine.epBulkOut.bEndpointAddress, 
		             CDC_Machine.epBulkOut.wMaxPacketSize, CDC_Machine.epBulkOut.bInterval);
  }
  else
  {
    pphost->usr_cb->DeviceNotSupported(); 
  }
  
  return USBH_OK ;
 
}


/**
  * @brief  USBH_CDC_InterfaceDeInit 
  *         De-Initialize interface by freeing host channels allocated to interface
  * @param  pdev: Selected device
  * @param  hdev: Selected device property
  * @retval None
  */
void USBH_CDC_InterfaceDeInit ( USB_OTG_CORE_HANDLE *pdev,
                                void *phost)
{	
  if ( CDC_Machine.hc_bulk_out)
  {
    USB_OTG_HC_Halt(pdev, CDC_Machine.hc_bulk_out);
    USBH_Free_Channel  (pdev, CDC_Machine.hc_bulk_out);
    CDC_Machine.hc_bulk_out = 0;     /* Reset the Channel as Free */
  }
   
  if ( CDC_Machine.hc_bulk_in)
  {
    USB_OTG_HC_Halt(pdev, CDC_Machine.hc_bulk_in);
    USBH_Free_Channel  (pdev, CDC_Machine.hc_bulk_in);
    CDC_Machine.hc_bulk_in = 0;     /* Reset the Channel as Free */
  }
  
  if ( CDC_Machine.hc_int_in)
  {
    USB_OTG_HC_Halt(pdev, CDC_Machine.hc_int_in);
    USBH_Free_Channel  (pdev, CDC_Machine.hc_int_in);
    CDC_Machine.hc_int_in = 0;     /* Reset the Channel as Free */
  } 
}

void usbh_cdc_set_state(CDC_Machine_TypeDef *pCDC_Machine, uint8_t state)
{
	if(pCDC_Machine->state != state)
	{
		//uprintf_default("USBH CDC state %d ==> state %d\n", pCDC_Machine->state, state);
		pCDC_Machine->state = state;
	}
}

void usbh_cdc_set_event(CDC_Machine_TypeDef *pCDC_Machine, uint8_t event)
{
	if(pCDC_Machine->event != event)
	{
		//uprintf_default("USBH CDC new event %d\n", event);
		pCDC_Machine->event = event;
	}
}

void usbh_cdc_action_tx(CDC_Machine_TypeDef *pCDC_Machine, USB_OTG_CORE_HANDLE *pdev)
{
	int16_t dataLength;
	
	dataLength = pCDC_Machine->TxDataLength - pCDC_Machine->TxDataOffset;
	if(dataLength > pCDC_Machine->epBulkOut.wMaxPacketSize)
		dataLength = pCDC_Machine->epBulkOut.wMaxPacketSize;

	if(dataLength > 0)
	{
		//uprintf_default("usbh_cdc_action_tx: length=%d\n", dataLength);
		USBH_BulkSendData (pdev,
		                 &pCDC_Machine->TxBuffer[pCDC_Machine->TxDataOffset], 
		                 dataLength , 
		                 pCDC_Machine->hc_bulk_out);

		//uprintf_default("CDC data out: length=%d data=0x%08x\n", dataLength, *(uint32_t *)&CDC_Machine.TxBuffer[CDC_Machine.TxDataOffset]);
		pCDC_Machine->TxDataPrevOffset = pCDC_Machine->TxDataOffset;
		pCDC_Machine->TxDataOffset += dataLength;
	}
}

void usbh_cdc_action_retx(CDC_Machine_TypeDef *pCDC_Machine, USB_OTG_CORE_HANDLE *pdev)
{
	int16_t dataLength;
	
	dataLength = pCDC_Machine->TxDataLength - pCDC_Machine->TxDataPrevOffset;
	if(dataLength > pCDC_Machine->epBulkOut.wMaxPacketSize)
		  dataLength = pCDC_Machine->epBulkOut.wMaxPacketSize;
	
	assert(dataLength > 0);
	
	USBH_BulkSendData (pdev,
                     &pCDC_Machine->TxBuffer[pCDC_Machine->TxDataPrevOffset], 
                     dataLength, 
                     pCDC_Machine->hc_bulk_out);
					 
	uprintf(UPRINT_INFO, UPRINT_BLK_DEF, "USBH ReTx: ep=%d len=%d offset=%d\n", 
			pCDC_Machine->hc_bulk_out, dataLength, pCDC_Machine->TxDataPrevOffset);
}

void usbh_cdc_action_tx_stall(CDC_Machine_TypeDef *pCDC_Machine, USB_OTG_CORE_HANDLE *pdev, void   *phost)
{
	USBH_Status status;
	
	uprintf(UPRINT_WARNING, UPRINT_BLK_DEF, "USBH_CDC_ERROR_OUT: bulk out ep stall\n");
	status = USBH_ClrFeature(pdev,
	                     phost,
	                     pCDC_Machine->epBulkOut.bEndpointAddress,
	                     pCDC_Machine->hc_bulk_out);
	if (status != USBH_OK)
	{
	  uprintf(UPRINT_WARNING, UPRINT_BLK_DEF, "USBH ERROR OUT recovery failure\n");
	}
}

void usbh_cdc_action_rx(CDC_Machine_TypeDef *pCDC_Machine, USB_OTG_CORE_HANDLE *pdev)
{
	if(pCDC_Machine->RxDataLength > pCDC_Machine->RxDataOffset)
	{
		//uprintf_default("usbh_cdc_action_rx: length=%d offset=%d\n", pCDC_Machine->RxDataLength, pCDC_Machine->RxDataOffset);
		
		/* after USBH_BulkReceiveData() executed, URB_Status will be URB_IDLE */
		USBH_BulkReceiveData (pdev,
						&pCDC_Machine->RxBuffer[pCDC_Machine->RxDataOffset], 
						pCDC_Machine->epBulkIn.wMaxPacketSize, 
						pCDC_Machine->hc_bulk_in);

		//uprintf_default("CDC data request in: length=%d offset=%d\n", dataLength, CDC_Machine.RxDataOffset);
		pCDC_Machine->RxDataOffset += pCDC_Machine->epBulkIn.wMaxPacketSize;

		pCDC_Machine->tLastRx = tick();
	}
}

void usbh_cdc_action_rx_stall(CDC_Machine_TypeDef *pCDC_Machine, USB_OTG_CORE_HANDLE *pdev, void   *phost)
{
	USBH_Status status;
	
	uprintf(UPRINT_WARNING, UPRINT_BLK_DEF, "USBH_CDC_ERROR_IN: bulk in ep stall\n");
	status = USBH_ClrFeature(pdev,
	                     phost,
	                     pCDC_Machine->epBulkIn.bEndpointAddress,
	                     pCDC_Machine->hc_bulk_in);
	if (status != USBH_OK)
	{
	  uprintf(UPRINT_WARNING, UPRINT_BLK_DEF, "USBH ERROR IN recovery failure\n");
	}
}

void usbh_cdc_action_rx_timeout(CDC_Machine_TypeDef *pCDC_Machine, USB_OTG_CORE_HANDLE *pdev)
{
	
}


/**
  * @brief  USBH_CDC_ClassRequest 
  *         This function will only initialize the MSC state machine
  * @param  pdev: Selected device
  * @param  hdev: Selected device property
  * @retval USBH_Status : Status of class request handled.
  */

static USBH_Status USBH_CDC_ClassRequest(USB_OTG_CORE_HANDLE *pdev , 
                                        void *phost)
{   
  
  USBH_Status status = USBH_OK ;
  
  return status; 
}

/**
  * @brief  USBH_CDC_Handle 
  *         CDC state machine handler: run in cdc task.
  * @param  pdev: Selected device
  * @param  hdev: Selected device property
  * @retval USBH_Status
  */
/*
2-4) USBH_MSC_Handle()
This callback is supposed to run class-specific protocol.

- interrupt IN endpoint
The interrupt IN EP returns Serial_State notification (DSR, DCD, break, parity error, etc)
If your application requires these status, run this EP.
Refer to usbh_hid_core.c, USBH_HID_Handle() for the handling of interrupt IN EP.
Put a transfer using USBH_InterruptReceiveData()
Poll the transfer completion using
if(HCD_GetURB_State(pdev , HID_Machine.hc_num_in) == URB_DONE)

- bulk IN/OUT endpoints
You may run the bulk IN/OUT EPs here, to pass data over RX/TX buffers,
or you may run these endpoints directly in your application object (USBH_USR_MSC_Application()).

USBH_BulkReceiveData(), USBH_BulkSendData() are used to receive/send a transaction.
When HCD_GetURB_State() returns URB_DONE, the transaction completes.
When HCD_GetURB_State() returns URB_NOTREADY, the endpoint is NAKing.
Refer to usbh_msc_bot.c, USBH_MSC_HandleBOTXfer(), USBH_MSC_BOT_DATAIN_STATE / USBH_MSC_BOT_DATAOUT_STATE

Lastly, USBH_MSC_Handle() calls pphost->usr_cb->UserApplication() to run the application object.
*/
	/*
	 * USB HOST state machine 
	IDLE -- (Tx Req)/(Tx) --> DATAOUT
	IDLE -- (Rx Req)/(Rx) --> DATAIN
	IDLE -- (poll interval)/(RX) --> DATAIN
	DATAOUT -- (Tx ACK Segment)/(Tx) --> DATAOUT
	DATAOUT -- (Tx ACK All)/(TxDoneCB) --> IDLE
	DATAOUT -- (Tx NACK)/(Re-Tx) --> DATAOUT
	DATAOUT -- (Tx Stall)/(Tx clear stall) --> IDLE
	DATAIN -- (Rx ACK segment)/(Rx) --> DATAIN
	DATAIN -- (Rx ACK all)/(RxDoneCB) --> IDLE
	DATAIN -- (Rx Stall)/(Rx clear stall) --> IDLE
	DATAIN -- (Rx timeout)/ --> IDLE
	*/

static USBH_Status USBH_CDC_Handle(USB_OTG_CORE_HANDLE *pdev , 
                                   void   *phost)
{
  USBH_Status status = USBH_BUSY;
  URB_STATE URB_Status = URB_IDLE;
  uint32_t tNow;
  
  /* first step: 
     1) only handle bulk endpoint and don't handle interrupt in endpoint.
	 2) bulk point state machine
	    IDLE
		DATAOUT
		DATAIN
		ERROROUT
		ERRORIN
   */
  if(HCD_IsDeviceConnected(pdev))
  {   
    switch(CDC_Machine.state)
    {
    case USBH_CDC_IDLE_STATE:
	  	switch(CDC_Machine.event)
		{
			case USBH_CDC_EVENT_TXREQ:
				usbh_cdc_action_tx(&CDC_Machine, pdev);
				// clear event after handling
				usbh_cdc_set_event(&CDC_Machine, USBH_CDC_EVENT_NULL);
				usbh_cdc_set_state(&CDC_Machine, USBH_CDC_DATAOUT_STATE);
				break;

			case USBH_CDC_EVENT_RXREQ:
				usbh_cdc_action_rx(&CDC_Machine, pdev);
				// clear event after handling
				usbh_cdc_set_event(&CDC_Machine, USBH_CDC_EVENT_NULL);
				usbh_cdc_set_state(&CDC_Machine, USBH_CDC_DATAIN_STATE);
				break;

			default:
				tNow = tick();
				if(tNow > CDC_Machine.tLastRx + USBH_CDC_RX_POLL)
				{
					CDC_Machine.RxDataOffset = 0;
					CDC_Machine.RxDataLength = CDC_Machine.epBulkIn.wMaxPacketSize;
					//uprintf_default("usb poll rx...\n");
					usbh_cdc_action_rx(&CDC_Machine, pdev);
					usbh_cdc_set_state(&CDC_Machine, USBH_CDC_DATAIN_STATE);
				}
				break;
		}
		break;
      
    case USBH_CDC_DATAIN_STATE:
		/* DATA IN stage */
		URB_Status =   HCD_GetURB_State(pdev, CDC_Machine.hc_bulk_in);
		if(URB_Status == URB_DONE)
		{
			if(CDC_Machine.RxDataLength > CDC_Machine.RxDataOffset) // event (Rx ACK segment)
			{
				usbh_cdc_action_rx(&CDC_Machine, pdev);
			}
			else // event (Rx ACK ALL)
			{
				/* call rx done callback */
				usbh_usr_cdc_rxdone(&CDC_Machine, CDC_Machine.RxBuffer, CDC_Machine.RxDataLength);
				CDC_Machine.tLastRx = tick();
				usbh_cdc_set_state(&CDC_Machine, USBH_CDC_IDLE_STATE);
			}
		}
		else if(URB_Status == URB_STALL) // event Rx stall
		{
			usbh_cdc_action_rx_stall(&CDC_Machine, pdev, phost);
			usbh_cdc_set_state(&CDC_Machine, USBH_CDC_IDLE_STATE);
		}
		else
		{
			/*
			HC_STATUS hc_status;
			uint8_t hc;
			uint32_t haint, haintmsk, hcchar, hcint, hcintmsk;

			hc = CDC_Machine.hc_bulk_in;
			hc_status = HCD_GetHCState(pdev, CDC_Machine.hc_bulk_in);
			uprintf_default("bulkin URB_Status=%d hc_status=%d data0=0x%08x data1=0x%08x\n", 
						URB_Status, hc_status, *(uint32_t *)&CDC_Machine.RxBuffer[0], *(uint32_t *)&CDC_Machine.RxBuffer[4]);
			
			haint = USB_OTG_READ_REG32((&pdev->regs.HREGS->HAINT));
			haintmsk = USB_OTG_READ_REG32(&pdev->regs.HREGS->HAINTMSK);
			hcchar = USB_OTG_READ_REG32(&pdev->regs.HC_REGS[hc]->HCCHAR);
			hcint = USB_OTG_READ_REG32(&pdev->regs.HC_REGS[hc]->HCINT);
			hcintmsk = USB_OTG_READ_REG32(&pdev->regs.HC_REGS[hc]->HCINTMSK);
			uprintf_default("haint=%08x haintmsk=%08x hcchar=%08x hcint=%08x hcintmsk=%08x\n",
				haint, haintmsk, hcchar, hcint, hcintmsk);
			*/

			/* after 10ms waiting, usually a nak received, back to idle state */
			usbh_cdc_set_state(&CDC_Machine, USBH_CDC_IDLE_STATE);
		} 
		break;   
      
    case USBH_CDC_DATAOUT_STATE:
		/* DATA OUT stage */
		URB_Status =   HCD_GetURB_State(pdev , CDC_Machine.hc_bulk_out);
		if(URB_Status == URB_DONE)
		{
			/* URB_DONE means a transfer has been done and a new can be started */
			if(CDC_Machine.TxDataLength > CDC_Machine.TxDataOffset) // event Tx ACK segment
			{
				usbh_cdc_action_tx(&CDC_Machine, pdev);
			}
			else // event Tx ACK all
			{
				usbh_cdc_set_state(&CDC_Machine, USBH_CDC_IDLE_STATE);
				usbh_usr_cdc_txdone(&CDC_Machine, CDC_Machine.TxBuffer, CDC_Machine.TxDataLength);
				CDC_Machine.TxDataLength = 0;
				CDC_Machine.TxDataOffset = 0;
				CDC_Machine.TxDataPrevOffset = 0;
			}
		}
		else if(URB_Status == URB_NOTREADY)
		{
			/* URB_NOTREADY means a re-transfer is needed */
			usbh_cdc_action_retx(&CDC_Machine, pdev);
		}
		else if(URB_Status == URB_STALL)
		{
			usbh_cdc_action_tx_stall(&CDC_Machine, pdev, phost);
			usbh_cdc_set_state(&CDC_Machine, USBH_CDC_IDLE_STATE);
		}
		break;
	  
    default:
      break; 
      
    }
  }
   return status;
}




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
