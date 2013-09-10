/**
  ******************************************************************************
  * @file    app.c
  * @author  MCD Application Team
  * @version V1.1.0
  * @date    19-March-2012
  * @brief   This file provides all the Application firmware functions.
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

/* Includes ------------------------------------------------------------------*/ 
#include <string.h>

#include "usbd_stb_core.h"
#include "usbd_usr.h"
#include "usb_conf.h"
#include "usbd_desc.h"
#include "usb_dcd_int.h"

/** @addtogroup STM32_USB_OTG_DEVICE_LIBRARY
  * @{
  */


/** @defgroup APP_VCP 
  * @brief Mass storage application module
  * @{
  */ 

/** @defgroup APP_VCP_Private_TypesDefinitions
  * @{
  */ 
/**
  * @}
  */ 


/** @defgroup APP_VCP_Private_Defines
  * @{
  */ 

/**
  * @}
  */ 


/** @defgroup APP_VCP_Private_Macros
  * @{
  */ 
/**
  * @}
  */ 


/** @defgroup APP_VCP_Private_Variables
  * @{
  */ 
  
#ifdef USB_OTG_HS_INTERNAL_DMA_ENABLED
  #if defined ( __ICCARM__ ) /*!< IAR Compiler */
    #pragma data_alignment=4   
  #endif
#endif /* USB_OTG_HS_INTERNAL_DMA_ENABLED */
   
__ALIGN_BEGIN USB_OTG_CORE_HANDLE    USB_OTG_dev[2] __ALIGN_END ;

usbd_device_data_t usbd_device_data[2];

/**
  * @}
  */ 


/** @defgroup APP_VCP_Private_FunctionPrototypes
  * @{
  */ 
/**
  * @}
  */ 


/** @defgroup APP_VCP_Private_Functions
  * @{
  */ 
void OTG_FS_IRQHandler(int irq)
{
  USBD_OTG_ISR_Handler (&USB_OTG_dev[1]);
}

void OTG_HS_IRQHandler(int irq)
{
  USBD_OTG_ISR_Handler (&USB_OTG_dev[0]);
}


/**
  * @brief  Program entry point
  * @param  None
  * @retval None
  */
int usb_app_init(void)
{
  /*!< At this stage the microcontroller clock setting is already configured, 
  this is done through SystemInit() function which is called from startup
  file (startup_stm32fxxx_xx.s) before to branch to application main.
  To reconfigure the default setting of SystemInit() function, refer to
  system_stm32fxxx.c file
  */  

  /* first device: HS */
  memset(&usbd_device_data[0], 0, sizeof(usbd_device_data_t));
  USB_OTG_dev[0].dev.usr_data = &usbd_device_data[0];
  usbd_device_data[0].core_id = USB_OTG_HS_CORE_ID;

  USBD_Init(&USB_OTG_dev[0],
            USB_OTG_HS_CORE_ID,
            &USR_desc, 
            &USBD_STB_cb, 
            &USR_cb);
  
  // ignored, for OTG_HS_IRQHandler is defined as week
  //isr_attach(OTG_HS_IRQn+16, OTG_HS_IRQHandler);
  //irq_unmask(OTG_HS_IRQn+16);
  //irq_mask(OTG_HS_IRQn+16);

  /* the second device: FS */
  memset(&usbd_device_data[1], 0, sizeof(usbd_device_data_t));
  USB_OTG_dev[1].dev.usr_data = &usbd_device_data[1];
  usbd_device_data[1].core_id = USB_OTG_FS_CORE_ID;
  
  USBD_Init(&USB_OTG_dev[1],
            USB_OTG_FS_CORE_ID,
            &USR_desc, 
            &USBD_STB_cb, 
            &USR_cb);

  // ignored, for OTG_FS_IRQHandler is defined as week
  //isr_attach(OTG_FS_IRQn+16, OTG_FS_IRQHandler);
  //irq_unmask(OTG_FS_IRQn+16);

  return 0;
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

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
