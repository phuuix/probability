# Makefile for GNU MAKE & GCC 2.8.0
# new directory "ports" created

PROJ_ROOTDIR = ../../..
include $(PROJ_ROOTDIR)/config.mk

VPATH = $(ARCH):core:api:netif:ports:core/ipv4

LWIPDIR = .
LWIPARCH = ports

CFLAGS:=$(CFLAGS) -DIPv4 -DLWIP_DEBUG \
	-I$(LWIPDIR)/include -I$(LWIPARCH)/include \
	-I$(PROJ_ROOTDIR)/$(BSP) -I$(PROJ_ROOTDIR)/stm32f2/CMSIS/Include \
	-I$(PROJ_ROOTDIR)/stm32f2/Include -I$(PROJ_ROOTDIR)/stm32f2/STM32F2xx_HAL_Driver/Inc

include Filelists.mk

# ARCHFILES: Architecture specific files.
ARCHFILES=$(LWIPARCH)/sys_arch.c

# LWIPFILES: All the above.
LWIPFILES=$(COREFILES) $(CORE4FILES) $(APIFILES) $(NETIFFILES) $(ARCHFILES)

S_FILES =
S_OBJS =
C_FILES = $(LWIPFILES)
C_OBJS = $(notdir $(C_FILES:.c=.o))


OBJS = $(S_OBJS) $(C_OBJS)
TARGET_LIB = $(LIB_PATH)/liblwip.a

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

