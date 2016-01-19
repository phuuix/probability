# Makefile for stm32f2xx bsp
# 
PROJ_ROOTDIR = ../..
include $(PROJ_ROOTDIR)/config.mk

CFLAGS += -I$(PROJ_ROOTDIR)/stm32f2/Include -I$(PROJ_ROOTDIR)/stm32f2/STM32F2xx_HAL_Driver/Inc -I$(PROJ_ROOTDIR)/stm32f2/CMSIS/Include \
	-I$(PROJ_ROOTDIR)/net/lwip-1.5beta/src/include/ -I$(PROJ_ROOTDIR)/net/lwip-1.5beta/src/ports/include

START_OBJ = startup_stm32f2xx.o
S_FILES = os_support_a.S
S_OBJS = $(notdir $(S_FILES:.S=.o))
C_FILES = console.c os_start.c os_support_c.c uart.c cli.c \
	cc2530ctrl.c netconf.c ethernetif.c system_stm32f2xx.c

C_OBJS = $(notdir $(C_FILES:.c=.o))

OBJS = $(S_OBJS) $(C_OBJS)

LIBS = $(LIB_PATH)/libdc.a $(LIB_PATH)/libkernel.a \
	$(LIB_PATH)/libbsp.a $(LIB_PATH)/liblwip.a $(LIB_PATH)/libhue.a \
	$(LIB_PATH)/libstm32f2hal.a #$(LIB_PATH)/libshell.a 
	
LIB_OPT = --start-group -ldc -lbsp -lkernel -ltty -llwip -lhue -lstm32f2hal -lgcc_thumb2 --end-group

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


