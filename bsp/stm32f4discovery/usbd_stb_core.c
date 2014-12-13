/**
  ******************************************************************************
  * @file    usbd_stb_core.c
  * @author  Puhui Xiong
  * @version V1.0.0
  * @date    31-01-2013
  * @brief   
  *           
  ******************************************************************************
  */ 

/* Includes ------------------------------------------------------------------*/
#include <string.h>
#include <stdlib.h>

#include "ipc.h"
//#include "samv_agent.h"
#include "usbd_conf.h"
#include "usbd_stb_core.h"
#include "usbd_desc.h"
#include "usbd_req.h"

#define USB_MDATASIZE 64
/*********************************************
   STB Device library callbacks
 *********************************************/
static uint8_t  usbd_stb_Init        (void  *pdev, uint8_t cfgidx);
static uint8_t  usbd_stb_DeInit      (void  *pdev, uint8_t cfgidx);
static uint8_t  usbd_stb_Setup       (void  *pdev, USB_SETUP_REQ *req);
static uint8_t  usbd_stb_EP0_RxReady  (void *pdev);
static uint8_t  usbd_stb_DataIn      (void *pdev, uint8_t epnum);
static uint8_t  usbd_stb_DataOut     (void *pdev, uint8_t epnum);
static uint8_t  usbd_stb_SOF         (void *pdev);

/*********************************************
   CDC specific management functions
 *********************************************/
static void Handle_USBAsynchXfer  (void *pdev);
static uint8_t  *USBD_stb_GetCfgDesc (uint8_t speed, uint16_t *length);
#ifdef USE_USB_OTG_HS  
static uint8_t  *USBD_stb_GetOtherCfgDesc (uint8_t speed, uint16_t *length);
#endif
/**
  * @}
  */ 

/** @defgroup usbd_stb_Private_Variables
  * @{
  */ 
extern uint8_t USBD_DeviceDesc   [USB_SIZ_DEVICE_DESC];
extern mbox_t samv_mbox;

/* CDC interface class callbacks structure */
USBD_Class_cb_TypeDef  USBD_STB_cb = 
{
  usbd_stb_Init,
  usbd_stb_DeInit,
  usbd_stb_Setup,
  NULL,                 /* EP0_TxSent, */
  usbd_stb_EP0_RxReady,
  usbd_stb_DataIn,
  usbd_stb_DataOut,
  usbd_stb_SOF,
  NULL,
  NULL,     
  USBD_stb_GetCfgDesc,
#ifdef USE_USB_OTG_HS   
  USBD_stb_GetOtherCfgDesc, /* use same cobfig as per FS */
#endif /* USE_USB_OTG_HS  */
};

#ifdef USB_OTG_HS_INTERNAL_DMA_ENABLED
  #if defined ( __ICCARM__ ) /*!< IAR Compiler */
    #pragma data_alignment=4   
  #endif
#endif /* USB_OTG_HS_INTERNAL_DMA_ENABLED */
/* USB CDC device Configuration Descriptor */
__ALIGN_BEGIN uint8_t usbd_stb_CfgDesc[USB_STB_CONFIG_DESC_SIZ]  __ALIGN_END =
{
  /*Configuration Descriptor*/
  0x09,   /* bLength: Configuration Descriptor size */
  USB_CONFIGURATION_DESCRIPTOR_TYPE,      /* bDescriptorType: Configuration */
  USB_STB_CONFIG_DESC_SIZ,                /* wTotalLength:no of returned bytes */
  0x00,
  0x01,   /* bNumInterfaces: 1 interface */
  0x01,   /* bConfigurationValue: Configuration value */
  0x00,   /* iConfiguration: Index of string descriptor describing the configuration */
  0x40,   /* bmAttributes: self powered */
  0x32,   /* MaxPower 0 mA */
  
  /*---------------------------------------------------------------------------*/
  
  /*Data class interface descriptor*/
  0x09,   /* bLength: Endpoint Descriptor size */
  USB_INTERFACE_DESCRIPTOR_TYPE,  /* bDescriptorType: */
  0x00,   /* bInterfaceNumber: Number of Interface */
  0x00,   /* bAlternateSetting: Alternate setting */
  0x02,   /* bNumEndpoints: Two endpoints used */
  0x00,   /* bInterfaceClass:  */
  0x00,   /* bInterfaceSubClass: */
  0x00,   /* bInterfaceProtocol: */
  0x00,   /* iInterface: */

  /*Endpoint IN Descriptor*/
  0x07,   /* bLength: Endpoint Descriptor size */
  USB_ENDPOINT_DESCRIPTOR_TYPE,      /* bDescriptorType: Endpoint */
  STB_IN_EP,                         /* bEndpointAddress */
  0x02,                              /* bmAttributes: Bulk */
  LOBYTE(STB_DATA_MAX_PACKET_SIZE),  /* wMaxPacketSize: */
  HIBYTE(STB_DATA_MAX_PACKET_SIZE),
  0x0A,                               /* bInterval: ignore for Bulk transfer */
  
  /*Endpoint OUT Descriptor*/
  0x07,   /* bLength: Endpoint Descriptor size */
  USB_ENDPOINT_DESCRIPTOR_TYPE,      /* bDescriptorType: Endpoint */
  STB_OUT_EP,                        /* bEndpointAddress */
  0x02,                              /* bmAttributes: Bulk */
  LOBYTE(STB_DATA_MAX_PACKET_SIZE),  /* wMaxPacketSize: */
  HIBYTE(STB_DATA_MAX_PACKET_SIZE),
  0x0A,                              /* bInterval: ignore for Bulk transfer */
} ;

/**
  * @}
  */ 

/** @defgroup usbd_stb_Private_Functions
  * @{
  */ 

/**
  * @brief  usbd_stb_Init
  *         Initilaize the CDC interface
  * @param  pdev: device instance
  * @param  cfgidx: Configuration index
  * @retval status
  */
static uint8_t  usbd_stb_Init (void  *pdev, 
                               uint8_t cfgidx)
{
  USB_OTG_CORE_HANDLE *udev = (USB_OTG_CORE_HANDLE *)pdev;
  usbd_device_data_t *usbd_data = (usbd_device_data_t *)udev->dev.usr_data;

  /* Open EP IN */
  DCD_EP_Open(pdev,
              STB_IN_EP,
              STB_DATA_IN_PACKET_SIZE,
              USB_OTG_EP_BULK);
  
  /* Open EP OUT */
  DCD_EP_Open(pdev,
              STB_OUT_EP,
              STB_DATA_OUT_PACKET_SIZE,
              USB_OTG_EP_BULK);
  
  /* Initialize the Interface physical components */
  usbd_data->APP_Rx_length = 0;
  usbd_data->APP_Rx_ptr_in = 0;
  usbd_data->APP_Rx_ptr_out = 0;
  usbd_data->USB_Tx_State = 0;
  usbd_data->usbd_stb_AltSet = 0;
  KPRINTF("usbd_stb_Init(): %s (core_id=%d)\n", 
  		usbd_data->core_id?"FSCORE":"HSCORE", usbd_data->core_id);

  /* Prepare Out endpoint to receive next packet */
  DCD_EP_PrepareRx(pdev,
                   STB_OUT_EP,
                   (uint8_t*)(usbd_data->USB_Rx_Buffer),
                   STB_DATA_OUT_PACKET_SIZE);
  
  return USBD_OK;
}

/**
  * @brief  usbd_stb_Init
  *         DeInitialize the CDC layer
  * @param  pdev: device instance
  * @param  cfgidx: Configuration index
  * @retval status
  */
static uint8_t  usbd_stb_DeInit (void  *pdev, 
                                 uint8_t cfgidx)
{
  USB_OTG_CORE_HANDLE *udev = (USB_OTG_CORE_HANDLE *)pdev;
  usbd_device_data_t *usbd_data = (usbd_device_data_t *)udev->dev.usr_data;
  
  /* Open EP IN */
  DCD_EP_Close(pdev,
              STB_IN_EP);
  
  /* Open EP OUT */
  DCD_EP_Close(pdev,
              STB_OUT_EP);

  /* Restore default state of the Interface physical components */
  KPRINTF("usbd_stb_DeInit(): core_id=%d\n", usbd_data->core_id);
  
  return USBD_OK;
}

/**
  * @brief  usbd_stb_Setup
  *         Handle the CDC specific requests
  * @param  pdev: instance
  * @param  req: usb requests
  * @retval status
  */
static uint8_t  usbd_stb_Setup (void  *pdev, 
                                USB_SETUP_REQ *req)
{
  uint16_t len=USB_STB_DESC_SIZ;
  uint8_t  *pbuf=usbd_stb_CfgDesc + 9;
  USB_OTG_CORE_HANDLE *udev = (USB_OTG_CORE_HANDLE *)pdev;
  usbd_device_data_t *usbd_data = (usbd_device_data_t *)udev->dev.usr_data;
  
  switch (req->bmRequest & USB_REQ_TYPE_MASK)
  {
    /* CDC Class Requests -------------------------------*/
  case USB_REQ_TYPE_CLASS :
  	  KPRINTF("usbd_stb_Setup() bmRequest is USB_REQ_TYPE_CLASS\n");
      
      return USBD_OK;
      
    default:
      USBD_CtlError (pdev, req);
      return USBD_FAIL;
    
      
      
    /* Standard Requests -------------------------------*/
  case USB_REQ_TYPE_STANDARD:
    switch (req->bRequest)
    {
    case USB_REQ_GET_DESCRIPTOR: 
      if( (req->wValue >> 8) == STB_DESCRIPTOR_TYPE)
      {
#ifdef USB_OTG_HS_INTERNAL_DMA_ENABLED
        pbuf = usbd_stb_Desc;   
#else
        pbuf = usbd_stb_CfgDesc + 9 + (9 * USBD_ITF_MAX_NUM);
#endif 
        len = MIN(USB_STB_DESC_SIZ , req->wLength);
      }
      
      USBD_CtlSendData (pdev, 
                        pbuf,
                        len);
      break;
      
    case USB_REQ_GET_INTERFACE :
      USBD_CtlSendData (pdev,
                        (uint8_t *)&usbd_data->usbd_stb_AltSet,
                        1);
      break;
      
    case USB_REQ_SET_INTERFACE :
      if ((uint8_t)(req->wValue) < USBD_ITF_MAX_NUM)
      {
        usbd_data->usbd_stb_AltSet = (uint8_t)(req->wValue);
      }
      else
      {
        /* Call the error management function (command will be nacked */
        USBD_CtlError (pdev, req);
      }
      break;
    }
  }
  return USBD_OK;
}

/**
  * @brief  usbd_stb_EP0_RxReady
  *         Data received on control endpoint
  * @param  pdev: device device instance
  * @retval status
  */
static uint8_t  usbd_stb_EP0_RxReady (void  *pdev)
{ 
  KPRINTF("usbd_stb_EP0_RxReady(): warning, unhandled!\n");
  
  return USBD_OK;
}

/**
  * @brief  usbd_stb_DataIn
  *         Data sent on non-control IN endpoint
  *         Called upon Tx Completed interrupt
  * @param  pdev: device instance
  * @param  epnum: endpoint number
  * @retval status
  */
static uint8_t  usbd_stb_DataIn (void *pdev, uint8_t epnum)
{
  uint16_t USB_Tx_ptr;
  uint16_t USB_Tx_length;
  USB_OTG_CORE_HANDLE *udev = (USB_OTG_CORE_HANDLE *)pdev;
  usbd_device_data_t *usbd_data = (usbd_device_data_t *)udev->dev.usr_data;

  //KPRINTF("usbd_stb_DataIn(): core_id=%d USB_Tx_State=%d APP_Rx_length=%d\n", 
  //		usbd_data->core_id, usbd_data->USB_Tx_State, usbd_data->APP_Rx_length);
  
  if (usbd_data->USB_Tx_State == 1)
  {
    if (usbd_data->APP_Rx_length == 0) 
    {
      usbd_data->USB_Tx_State = 0;
    }
    else 
    {
      if (usbd_data->APP_Rx_length > STB_DATA_IN_PACKET_SIZE){
        USB_Tx_ptr = usbd_data->APP_Rx_ptr_out;
        USB_Tx_length = STB_DATA_IN_PACKET_SIZE;
        
        usbd_data->APP_Rx_ptr_out += STB_DATA_IN_PACKET_SIZE;
        usbd_data->APP_Rx_length -= STB_DATA_IN_PACKET_SIZE;    
      }
      else 
      {
        USB_Tx_ptr = usbd_data->APP_Rx_ptr_out;
        USB_Tx_length = usbd_data->APP_Rx_length;
        
        usbd_data->APP_Rx_ptr_out += usbd_data->APP_Rx_length;
        usbd_data->APP_Rx_length = 0;
      }
      
      /* Prepare the available data buffer to be sent on IN endpoint */
      DCD_EP_Tx (pdev,
                 STB_IN_EP,
                 (uint8_t*)&usbd_data->APP_Rx_Buffer[USB_Tx_ptr],
                 USB_Tx_length);
    }
  }  
  
  return USBD_OK;
}

/**
  * @brief  usbd_stb_DataOut
  *         Data received on non-control Out endpoint
  * @param  pdev: device instance
  * @param  epnum: endpoint number
  * @retval status
  */
static uint8_t  usbd_stb_DataOut (void *pdev, uint8_t epnum)
{      
  uint16_t USB_Rx_Cnt;
  USB_OTG_CORE_HANDLE *udev = (USB_OTG_CORE_HANDLE *)pdev;
  usbd_device_data_t *usbd_data = (usbd_device_data_t *)udev->dev.usr_data;
  
  /* Get the received data buffer and update the counter */
  USB_Rx_Cnt = ((USB_OTG_CORE_HANDLE*)pdev)->dev.out_ep[epnum].xfer_count;
  
  /* USB data will be immediately processed, this allow next USB traffic being 
     NAKed till the end of the application Xfer */
  assert(USB_Rx_Cnt < USB_MDATASIZE);
  assert(usbd_data);
  
  //KPRINTF("usbd_stb_DataOut: core_id=%d buf=0x%x len=%d\n", 
  //		usbd_data->core_id, usbd_data->USB_Rx_Buffer, USB_Rx_Cnt);
  //dump_buffer(usbd_data->USB_Rx_Buffer, USB_Rx_Cnt);
#if 0
  /* post to samv */
  samv_mail_t mail;

  mail.bufptr = malloc(USB_MDATASIZE);
  if(mail.bufptr)
  {
    memcpy(mail.bufptr, usbd_data->USB_Rx_Buffer, USB_Rx_Cnt);
    /* convert uart port to samv port */
	mail.event = SAMVA_EVENT_HOSTMSG;
	mail.port = usbd_data->core_id+1;
	mail.length = USB_Rx_Cnt;
    if(USB_Rx_Cnt==0 || mbox_post(&samv_mbox, (uint32_t *)&mail) != ROK)
      free(mail.bufptr);
  }
#endif
  /* Prepare Out endpoint to receive next packet */
  DCD_EP_PrepareRx(pdev,
                   STB_OUT_EP,
                   (uint8_t*)(usbd_data->USB_Rx_Buffer),
                   STB_DATA_OUT_PACKET_SIZE);

  return USBD_OK;
}

/**
  * @brief  usbd_stb_SOF
  *         Start Of Frame event management (usually per 1ms?)
  * @param  pdev: instance
  * @param  epnum: endpoint number
  * @retval status
  */
static uint8_t  usbd_stb_SOF (void *pdev)
{
   USB_OTG_CORE_HANDLE *udev = (USB_OTG_CORE_HANDLE *)pdev;
  //KPRINTF("usbd_stb_SOF() FrameCount=%d\n", FrameCount);
  
  /* Check the data to be sent through IN pipe */
  if(udev->dev.device_status == USB_OTG_CONFIGURED)
    Handle_USBAsynchXfer(pdev);
  
  return USBD_OK;
}

/**
  * @brief  Handle_USBAsynchXfer
  *         Send data to USB
  * @param  pdev: instance
  * @retval None
  */
static void Handle_USBAsynchXfer (void *pdev)
{
  uint16_t USB_Tx_ptr;
  uint16_t USB_Tx_length;
  USB_OTG_CORE_HANDLE *udev = (USB_OTG_CORE_HANDLE *)pdev;
  usbd_device_data_t *usbd_data = (usbd_device_data_t *)udev->dev.usr_data;
  
  if(usbd_data->USB_Tx_State != 1)
  {
    if (usbd_data->APP_Rx_ptr_out == APP_RX_DATA_SIZE)
    {
      usbd_data->APP_Rx_ptr_out = 0;
    }
    
    if(usbd_data->APP_Rx_ptr_out == usbd_data->APP_Rx_ptr_in) 
    {
      usbd_data->USB_Tx_State = 0; 
      return;
    }
    
    if(usbd_data->APP_Rx_ptr_out > usbd_data->APP_Rx_ptr_in) /* rollback */
    { 
      usbd_data->APP_Rx_length = APP_RX_DATA_SIZE - usbd_data->APP_Rx_ptr_out;
    
    }
    else 
    {
      usbd_data->APP_Rx_length = usbd_data->APP_Rx_ptr_in - usbd_data->APP_Rx_ptr_out;
     
    }
#ifdef USB_OTG_HS_INTERNAL_DMA_ENABLED
     usbd_data->APP_Rx_length &= ~0x03;
#endif /* USB_OTG_HS_INTERNAL_DMA_ENABLED */
    
    if (usbd_data->APP_Rx_length > STB_DATA_IN_PACKET_SIZE)
    {
      USB_Tx_ptr = usbd_data->APP_Rx_ptr_out;
      USB_Tx_length = STB_DATA_IN_PACKET_SIZE;
      
      usbd_data->APP_Rx_ptr_out += STB_DATA_IN_PACKET_SIZE;	
      usbd_data->APP_Rx_length -= STB_DATA_IN_PACKET_SIZE;
    }
    else
    {
      USB_Tx_ptr = usbd_data->APP_Rx_ptr_out;
      USB_Tx_length = usbd_data->APP_Rx_length;
      
      usbd_data->APP_Rx_ptr_out += usbd_data->APP_Rx_length;
      usbd_data->APP_Rx_length = 0;
    }
    usbd_data->USB_Tx_State = 1; 

    //KPRINTF("Handle_USBAsynchXfer(): core_id=%d, USB_Tx_State=0 USB_Tx_length=%d\n", 
	//		usbd_data->core_id, USB_Tx_length);
	//dump_buffer(&usbd_data->APP_Rx_Buffer[USB_Tx_ptr], USB_Tx_length);
	
    DCD_EP_Tx (pdev,
               STB_IN_EP,
               (uint8_t*)&usbd_data->APP_Rx_Buffer[USB_Tx_ptr],
               USB_Tx_length);
  }  
  
}

/**
  * @brief  USBD_stb_GetCfgDesc 
  *         Return configuration descriptor
  * @param  speed : current device speed
  * @param  length : pointer data length
  * @retval pointer to descriptor buffer
  */
static uint8_t  *USBD_stb_GetCfgDesc (uint8_t speed, uint16_t *length)
{
  *length = sizeof (usbd_stb_CfgDesc);
  return usbd_stb_CfgDesc;
}

/**
  * @brief  USBD_stb_GetCfgDesc 
  *         Return configuration descriptor
  * @param  speed : current device speed
  * @param  length : pointer data length
  * @retval pointer to descriptor buffer
  */
#ifdef USE_USB_OTG_HS 
static uint8_t  *USBD_stb_GetOtherCfgDesc (uint8_t speed, uint16_t *length)
{
  *length = 0;
  return NULL;
}
#endif
/**
  * @}
  */ 

/* application send out pacet over USB lines */
extern usbd_device_data_t usbd_device_data[];
uint32_t usbd_stb_apptx(uint8_t port, uint8_t *buffer, uint16_t length)
{
	uint32_t f;
	usbd_device_data_t *usbd_data;
	assert(length < APP_RX_DATA_SIZE);
	assert(port < 2);

	usbd_data = &usbd_device_data[port];

	f = bsp_fsave();
	if(usbd_data->USB_Tx_State == 0)
	{
		/* we only issue tx command when APP_Rx_Buffer is empty */
		// TODO: fix this limit to improve performce.
		memcpy(usbd_data->APP_Rx_Buffer, buffer, length);
		usbd_data->APP_Rx_length = 0;
		usbd_data->APP_Rx_ptr_in = length;
		usbd_data->APP_Rx_ptr_out = 0;

		bsp_frestore(f);
		return length;
	}
	bsp_frestore(f);

	/* usb busy */
	KPRINTF("Warning: USB device busy\n");
	return 0;
}

/**
  * @}
  */ 

/**
  * @}
  */ 

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
