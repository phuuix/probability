# Makefile for hostsim bsp
# 
BSP = hostsim
PROJ_ROOTDIR = ../..
include $(PROJ_ROOTDIR)/config.mk

START_OBJ = 
S_FILES = 
S_OBJS = $(notdir $(S_FILES:.S=.o))
C_FILES = console.c os_start.c os_support_c.c
C_OBJS = $(notdir $(C_FILES:.c=.o))

OBJS = $(S_OBJS) $(C_OBJS)

LIBS = $(LIB_PATH)/libkernel.a $(LIB_PATH)/libbsp.a 
#$(LIB_PATH)/libdc.a  $(LIB_PATH)/libshell.a $(LIB_PATH)/liblwip4.a
	
LIB_OPT = --start-group -lbsp -lkernel -ltty --end-group

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


