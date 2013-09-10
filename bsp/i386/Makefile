# Makefile for i386 bsp
# 
PROJ_ROOTDIR = ../..
include $(PROJ_ROOTDIR)/config.mk

START_OBJ = boot.o
S_FILES = idt.s
S_OBJS = idt.o
#C_FILES = $(wildcard *.c)
C_FILES = i386_cons.c i386_hw.c i386_stub.c i386_tasksw.c irq.c isr.c keyboard.c \
	kprintf.c lock.c pci.c startup.c tty_pc_drv.c gdb_udp.c
C_OBJS = $(notdir $(C_FILES:.c=.o))

OBJS = $(S_OBJS) $(C_OBJS)

LIBS = $(LIB_PATH)/libdc.a \
	$(LIB_PATH)/libkernel.a \
	$(LIB_PATH)/libbsp.a \
	$(LIB_PATH)/libtty.a \
	$(LIB_PATH)/libshell.a
	#$(LIB_PATH)/liblwip4.a
	
#LIB_OPT = --start-group -ldc -lbsp -lkernel -lgcc -lshell -ltty -llwip4 --end-group
LIB_OPT = --start-group -ldc -lbsp -lkernel -lgcc -lshell -ltty --end-group

all: dooloo 
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

$(LIB_PATH)/libbsp.a : $(OBJS)
	$(AR) rs $@ $?

bochs: dooloo
	mdel a:\dooloo
	mcopy dooloo a:\dooloo
			

