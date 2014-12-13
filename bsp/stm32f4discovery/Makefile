# Makefile for stm32f2xx bsp
# 
PROJ_ROOTDIR = ../..
include $(PROJ_ROOTDIR)/config.mk

START_OBJ = startup_stm32f4xx.o
S_FILES = os_support_a.S
S_OBJS = $(notdir $(S_FILES:.S=.o))
C_FILES = console.c os_start.c uart.c cli.c os_support_c.c \
	stm32f4xx_tim.c system_stm32f4xx.c stm32f4xx_rcc.c misc.c stm32f4xx_exti.c stm32f4xx_spi.c \
	stm32f4xx_usart.c stm32f4xx_gpio.c stm32f4xx_syscfg.c stm32f4_discovery_lis302dl.c \
	usb_app.c usb_bsp.c usbd_desc.c usbd_usr.c \
	usb_core.c usb_dcd.c usb_dcd_int.c \
	usbd_core.c usbd_ioreq.c usbd_req.c usbd_stb_core.c
C_OBJS = $(notdir $(C_FILES:.c=.o))

OBJS = $(S_OBJS) $(C_OBJS)

LIBS = $(LIB_PATH)/libdc.a $(LIB_PATH)/libkernel.a \
	$(LIB_PATH)/libbsp.a #$(LIB_PATH)/libshell.a $(LIB_PATH)/liblwip4.a
	
LIB_OPT = --start-group -ldc -lbsp -lkernel -ltty -lgcc_thumb2 --end-group

all: dooloo dooloo.bin
.PHONY: all

clean :
	$(RM) *.o
	$(RM) $(LIB_PATH)/libbsp.a .depend dooloo dooloo.bin

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


