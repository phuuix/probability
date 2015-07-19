/**
  ******************************************************************************
  * @file    stm32f2x7_eth_conf.h
  * @author  MCD Application Team
  * @version V1.1.0
  * @date    07-October-2011
  * @brief   Configuration file for the STM32F2x7 Ethernet driver. 
  ******************************************************************************
  * @attention
  *
  * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
  * TIME. AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY
  * DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
  * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
  * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  * <h2><center>&copy; COPYRIGHT 2011 STMicroelectronics</center></h2>
  ******************************************************************************
  */
/* Puhuix: 2014-10-06 change for PHY LAN8720 */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __STM32F2x7_ETH_CONF_H
#define __STM32F2x7_ETH_CONF_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f2xx.h"

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/

/* Uncomment the line below when using time stamping and/or IPv4 checksum offload */
#define USE_ENHANCED_DMA_DESCRIPTORS

/* Uncomment the line below if you want to use user defined Delay function
   (for precise timing), otherwise default _eth_delay_ function defined within
   the Ethernet driver is used (less precise timing) */
//#define USE_Delay

#ifdef USE_Delay
  #include "main.h"                /* Header file where the Delay function prototype is exported */  
  #define _eth_delay_    Delay     /* User can provide more timing precise _eth_delay_ function */
#else
  #define _eth_delay_    ETH_Delay /* Default _eth_delay_ function with less precise timing */
#endif


/* Uncomment the line below to allow custom configuration of the Ethernet driver buffers */    
//#define CUSTOM_DRIVER_BUFFERS_CONFIG   

#ifdef  CUSTOM_DRIVER_BUFFERS_CONFIG
/* Redefinition of the Ethernet driver buffers size and count */   
 #define ETH_RX_BUF_SIZE    ETH_MAX_PACKET_SIZE /* buffer size for receive */
 #define ETH_TX_BUF_SIZE    ETH_MAX_PACKET_SIZE /* buffer size for transmit */
 #define ETH_RXBUFNB        20                  /* 20 Rx buffers of size ETH_RX_BUF_SIZE */
 #define ETH_TXBUFNB        5                   /* 5  Tx buffers of size ETH_TX_BUF_SIZE */
#endif


/* PHY configuration section **************************************************/
/* PHY Reset delay */ 
#define PHY_RESET_DELAY    ((uint32_t)0x000FFFFF) 
/* PHY Configuration delay */ 
#define PHY_CONFIG_DELAY   ((uint32_t)0x00FFFFFF)

#define PHY_8720_BCR													0
#define PHY_8720_BSR													1

/* definition for PHY_8720_BCR */
#define PHY_8720_Reset                       ((uint16_t)0x8000)      /*!< PHY Reset */
#define PHY_8720_Loopback                    ((uint16_t)0x4000)      /*!< Select loop-back mode */
#define PHY_8720_FULLDUPLEX_100M             ((uint16_t)0x2100)      /*!< Set the full-duplex mode at 100 Mb/s */
#define PHY_8720_HALFDUPLEX_100M             ((uint16_t)0x2000)      /*!< Set the half-duplex mode at 100 Mb/s */
#define PHY_8720_FULLDUPLEX_10M              ((uint16_t)0x0100)      /*!< Set the full-duplex mode at 10 Mb/s */
#define PHY_8720_HALFDUPLEX_10M              ((uint16_t)0x0000)      /*!< Set the half-duplex mode at 10 Mb/s */
#define PHY_8720_AutoNegotiation             ((uint16_t)0x1000)      /*!< Enable auto-negotiation function */
#define PHY_8720_Restart_AutoNegotiation     ((uint16_t)0x0200)      /*!< Restart auto-negotiation function */
#define PHY_8720_Powerdown                   ((uint16_t)0x0800)      /*!< Select the power down mode */
#define PHY_8720_Isolate                     ((uint16_t)0x0400)      /*!< Isolate PHY from MII */
/* definition for PHY_8720_BSR */
#define PHY_8720_BSR_100BT4                  ((uint16_t)0x8000)
#define PHY_8720_BSR_100BT_FD                ((uint16_t)0x4000)
#define PHY_8720_BSR_100BT_HD                ((uint16_t)0x2000)
#define PHY_8720_BSR_10BT_FD                 ((uint16_t)0x1000)
#define PHY_8720_BSR_10BT_HD                 ((uint16_t)0x0800)


/* The PHY status register value change from a PHY to another, so the user have 
   to update this value depending on the used external PHY */
#define PHY_SR    ((uint16_t)31) /* Value for LAN8720 PHY */

/* The Speed and Duplex mask values change from a PHY to another, so the user
   have to update this value depending on the used external PHY */
#define PHY_SPEED_STATUS            (0x04) /* Value for LAN8720 PHY */
#define PHY_DUPLEX_STATUS           (0x10) /* Value for LAN8720 PHY */

   
/* Exported macro ------------------------------------------------------------*/

/* MII and RMII mode selection, for STM322xG-EVAL Board(MB786) RevB ***********/
#define RMII_MODE  // User have to provide the 50 MHz clock by soldering a 50 MHz
                     // oscillator (ref SM7745HEV-50.0M or equivalent) on the U3
                     // footprint located under CN3 and also removing jumper on JP5. 
                     // This oscillator is not provided with the board. 
                     // For more details, please refer to STM3220G-EVAL evaluation
                     // board User manual (UM1057).

                                     
//#define MII_MODE

/* Uncomment the define below to clock the PHY from external 25MHz crystal (only for MII mode) */
#ifdef 	MII_MODE
 #define PHY_CLOCK_MCO
#endif


/* Exported functions ------------------------------------------------------- */  

#ifdef __cplusplus
}
#endif

#endif /* __STM32F2x7_ETH_CONF_H */


/******************* (C) COPYRIGHT 2011 STMicroelectronics *****END OF FILE****/

