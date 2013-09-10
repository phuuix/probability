# Makefile for GNU MAKE & GCC 

PROJ_ROOTDIR = ..
include $(PROJ_ROOTDIR)/config.mk

# Please let $(ARCH) at first
VPATH = $(ARCH):lib:libcruft

S_FILES =
S_OBJS =
C_FILES = $(wildcard lib/*.c) $(wildcard libcruft/*.c)
C_OBJS = $(notdir $(C_FILES:.c=.o))


OBJS = $(S_OBJS) $(C_OBJS)
TARGET_LIB = $(LIB_PATH)/libdc.a

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

