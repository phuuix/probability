# Makefile for ep7312 bsp
# 
PROJ_ROOTDIR = ../..
include $(PROJ_ROOTDIR)/config.mk

START_OBJ = start.o
S_FILES = os_support_a.S
S_OBJS = $(notdir $(S_FILES:.S=.o))
C_FILES = console.c cpu.c interrupts.c kprintf.c lpc221x.c os_start.c \
	os_support_c.c serial.c gdbserver_arm.c cli.c
C_OBJS = $(notdir $(C_FILES:.c=.o))

OBJS = $(S_OBJS) $(C_OBJS)

LIBS = $(LIB_PATH)/libdc.a $(LIB_PATH)/libkernel.a $(LIB_PATH)/libsamv.a \
	$(LIB_PATH)/libbsp.a #$(LIB_PATH)/libshell.a $(LIB_PATH)/liblwip4.a
	
LIB_OPT = --start-group -ldc -lbsp -lkernel -ltty -lsamv -lgcc_armcs --end-group

all: dooloo dooloo.bin
.PHONY: all

clean :
	$(RM) *.o
	$(RM) $(LIB_PATH)/libbsp.a .depend 

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


