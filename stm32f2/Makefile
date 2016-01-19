# Makefile for GNU MAKE & GCC 2.8.0

PROJ_ROOTDIR = ..
include $(PROJ_ROOTDIR)/config.mk

HALDIR = STM32F2xx_HAL_Driver
USBDDIR = STM32_USB_Device_Library
USBHDIR = STM32_USB_Host_Library

LIBHAL = $(LIB_PATH)/libstm32f2hal.a
LIBUSBD = $(LIB_PATH)/libstm32f2usbd.a
LIBUSBH = $(LIB_PATH)/libstm32f2usbh.a

VPATH = $(HALDIR)/Src:$(USBDDIR)/Core/Src:$(USBHDIR)/Core/Src:$(USBHDIR)/Class/CDC/Src

HAL_CFILES = $(HALDIR)/Src/stm32f2xx_hal.c \
		$(HALDIR)/Src/stm32f2xx_hal_adc.c \
		$(HALDIR)/Src/stm32f2xx_hal_adc_ex.c \
		$(HALDIR)/Src/stm32f2xx_hal_can.c \
		$(HALDIR)/Src/stm32f2xx_hal_cortex.c \
		$(HALDIR)/Src/stm32f2xx_hal_crc.c \
		$(HALDIR)/Src/stm32f2xx_hal_cryp.c \
		$(HALDIR)/Src/stm32f2xx_hal_dac.c \
		$(HALDIR)/Src/stm32f2xx_hal_dac_ex.c \
		$(HALDIR)/Src/stm32f2xx_hal_dcmi.c \
		$(HALDIR)/Src/stm32f2xx_hal_dma.c \
		$(HALDIR)/Src/stm32f2xx_hal_dma_ex.c \
        $(HALDIR)/Src/stm32f2xx_hal_eth.c \
		$(HALDIR)/Src/stm32f2xx_hal_flash.c \
		$(HALDIR)/Src/stm32f2xx_hal_flash_ex.c \
		$(HALDIR)/Src/stm32f2xx_hal_gpio.c \
		$(HALDIR)/Src/stm32f2xx_hal_hash.c \
		$(HALDIR)/Src/stm32f2xx_hal_hcd.c \
		$(HALDIR)/Src/stm32f2xx_hal_i2c.c \
		$(HALDIR)/Src/stm32f2xx_hal_i2s.c \
		$(HALDIR)/Src/stm32f2xx_hal_irda.c \
		$(HALDIR)/Src/stm32f2xx_hal_iwdg.c \
		$(HALDIR)/Src/stm32f2xx_hal_nand.c \
		$(HALDIR)/Src/stm32f2xx_hal_nor.c \
		$(HALDIR)/Src/stm32f2xx_hal_pccard.c \
		$(HALDIR)/Src/stm32f2xx_hal_pcd.c \
		$(HALDIR)/Src/stm32f2xx_hal_pcd_ex.c \
		$(HALDIR)/Src/stm32f2xx_hal_pwr.c \
		$(HALDIR)/Src/stm32f2xx_hal_pwr_ex.c \
		$(HALDIR)/Src/stm32f2xx_hal_rcc.c \
		$(HALDIR)/Src/stm32f2xx_hal_rcc_ex.c \
		$(HALDIR)/Src/stm32f2xx_hal_rng.c \
		$(HALDIR)/Src/stm32f2xx_hal_rtc.c \
		$(HALDIR)/Src/stm32f2xx_hal_rtc_ex.c \
		$(HALDIR)/Src/stm32f2xx_hal_sd.c \
		$(HALDIR)/Src/stm32f2xx_hal_smartcard.c \
		$(HALDIR)/Src/stm32f2xx_hal_spi.c \
		$(HALDIR)/Src/stm32f2xx_hal_sram.c \
		$(HALDIR)/Src/stm32f2xx_hal_tim.c \
		$(HALDIR)/Src/stm32f2xx_hal_tim_ex.c \
		$(HALDIR)/Src/stm32f2xx_hal_uart.c \
		$(HALDIR)/Src/stm32f2xx_hal_usart.c \
		$(HALDIR)/Src/stm32f2xx_hal_wwdg.c \
		$(HALDIR)/Src/stm32f2xx_ll_fsmc.c \
		$(HALDIR)/Src/stm32f2xx_ll_sdmmc.c \
		$(HALDIR)/Src/stm32f2xx_ll_usb.c
USBD_CFILES = $(USBDDIR)/Core/Src/usbd_core.c \
		$(USBDDIR)/Core/Src/usbd_ctlreq.c \
		$(USBDDIR)/Core/Src/usbd_ioreq.c
USBH_CFILES = $(USBHDIR)/Core/Src/usbh_core.c \
		$(USBHDIR)/Core/Src/usbh_ctlreq.c \
		$(USBHDIR)/Core/Src/usbh_ioreq.c \
		$(USBHDIR)/Core/Src/usbh_pipes.c \
		$(USBHDIR)/Class/CDC/Src/usbh_cdc.c

HAL_OBJS = $(notdir $(HAL_CFILES:.c=.o))
USBD_OBJS = $(notdir $(USBD_CFILES:.c=.o))
USBH_OBJS = $(notdir $(USBH_CFILES:.c=.o))


S_FILES =
S_OBJS =
C_FILES = $(HAL_CFILES) $(USBH_CFILES) #$(USBD_CFILES)
C_OBJS = $(notdir $(C_FILES:.c=.o))
OBJS = $(S_OBJS) $(C_OBJS)
TARGET_LIB = $(LIBHAL) $(LIBUSBH) #$(LIBUSBD)
# below 1 lines to be removed
# CFLAGS   := $(DBGFLAGS) $(OPTFLAGS) $(RELFLAGS) -nostdinc -fno-builtin -g -Wall -finline-functions -fno-stack-protector -mapcs -mcpu=cortex-m3 -mthumb -I$(PROJ_ROOTDIR)/libc/include
CFLAGS += -IInclude -ICMSIS/Include -I$(HALDIR)/Inc -I$(USBDDIR)/Core/Inc -I$(USBHDIR)/Core/Inc -I$(USBHDIR)/Class/CDC/Inc

all: $(TARGET_LIB)
.PHONY: all

clean :
	$(RM) *.o
	$(RM) $(TARGET_LIB) .depend

dep : .depend

include .depend

.depend: $(C_FILES)
	$(CC) $(CFLAGS) -M $^ > $@

$(LIBHAL) : $(HAL_OBJS)
	$(AR) rs $@ $?

$(LIBUSBD) : $(USBD_OBJS)
	$(AR) rs $@ $?
	
$(LIBUSBH) : $(USBH_OBJS)
	$(AR) rs $@ $?
