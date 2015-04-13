# Makefile for GNU MAKE & GCC 2.8.0

PROJ_ROOTDIR = ..
include $(PROJ_ROOTDIR)/config.mk

VPATH = $(ARCH):lib

S_FILES =
S_OBJS =
C_FILES = httpserver-netconn.c hue.c zll_controller.c zllSocCmd.c cJSON.c hue_json_builder.c 
C_OBJS = $(notdir $(C_FILES:.c=.o))


OBJS = $(S_OBJS) $(C_OBJS)
TARGET_LIB = $(LIB_PATH)/libhue.a
CFLAGS += -I../net/lwip-1.4.1/src/include -I../net/lwip-1.4.1/src/include/ipv4 -I../net/lwip-1.4.1/src/ports/include

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

