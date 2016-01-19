# Makefile for GNU MAKE & GCC 2.8.0

PROJ_ROOTDIR = ..
include $(PROJ_ROOTDIR)/config.mk

VPATH = $(ARCH):lib

S_FILES =
S_OBJS =
C_FILES = httpserver-netconn.c hue.c zll_controller.c zllSocCmd.c cJSON.c hue_json_builder.c http_parser.c ssdp.c
C_OBJS = $(notdir $(C_FILES:.c=.o))


OBJS = $(S_OBJS) $(C_OBJS)
TARGET_LIB = $(LIB_PATH)/libhue.a
CFLAGS += -I../net/lwip-1.5beta/src/include -I../net/lwip-1.5beta/src/include/ipv4 -I../net/lwip-1.5beta/src/ports/include \
	-I../stm32f2/Include -I../stm32f2/CMSIS/Include -I../stm32f2/STM32F2xx_HAL_Driver/Inc

all: $(TARGET_LIB)
.PHONY: all

clean :
	$(RM) *.o
	$(RM) $(TARGET_LIB) .depend

dep : .depend

include .depend

.depend: $(C_FILES)
	$(CC) $(CFLAGS) -M $^ > $@

$(TARGET_LIB) : $(OBJS)
	$(AR) rs $(TARGET_LIB) $?

