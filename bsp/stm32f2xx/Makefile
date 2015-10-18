# Makefile for stm32f2xx bsp
# 
PROJ_ROOTDIR = ../..
include $(PROJ_ROOTDIR)/config.mk

START_OBJ = startup_stm32f2xx.o
S_FILES = os_support_a.S
S_OBJS = $(notdir $(S_FILES:.S=.o))
C_FILES = console.c os_start.c stm32f2xx_usart.c stm32f2xx_gpio.c stm32f2xx_syscfg.c \
	os_support_c.c system_stm32f2xx.c stm32f2xx_tim.c stm32f2x7_eth.c stm32f2x7_eth_bsp.c \
	uart.c core_cm3.c cli.c stm32f2xx_rcc.c misc.c stm32f2xx_exti.c \
	usb_core.c usbh_bsp.c\
	usbh_core.c usbh_cdc_core.c usbh_hcs.c usbh_ioreq.c usbh_stdreq.c usbh_usr.c \
	usb_hcd.c usb_hcd_int.c cc2530ctrl.c
	
# usb device files: 
#  usb_app.c usbd_bsp.c usbd_desc.c usbd_usr.c usb_dcd.c usb_dcd_int.c \
#  usbd_core.c usbd_ioreq.c usbd_req.c usbd_stb_core.c \

C_OBJS = $(notdir $(C_FILES:.c=.o))

OBJS = $(S_OBJS) $(C_OBJS)

LIBS = $(LIB_PATH)/libdc.a $(LIB_PATH)/libkernel.a \
	$(LIB_PATH)/libbsp.a $(LIB_PATH)/liblwip.a $(LIB_PATH)/libhue.a #$(LIB_PATH)/libshell.a 
	
LIB_OPT = --start-group -ldc -lbsp -lkernel -ltty -llwip -lhue -lgcc_thumb2 --end-group

all: dooloo dooloo.bin
.PHONY: all

clean :
	$(RM) *.o
	$(RM) $(LIB_PATH)/libbsp.a .depend 
	$(RM) dooloo dooloo.bin

dep : .depend

include .depend

.depend: $(C_FILES)
	$(CC) $(CFLAGS) -M $^ > $@

dooloo : $(START_OBJ) $(LIBS)
	$(LD) $(LDFLAGS) $(START_OBJ) $(LIB_OPT) -o $@

dooloo.bin : dooloo
	$(OBJCOPY) ${OBJCFLAGS} -O binary $< $@

$(LIB_PATH)/libbsp.a : $(OBJS)
	$(AR) rs $@ $?


