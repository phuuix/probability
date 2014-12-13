# Makefile for kernel
# 
PROJ_ROOTDIR = ..
include $(PROJ_ROOTDIR)/config.mk

S_FILES = $(wildcard *.s)
S_OBJS = $(notdir $(S_FILES:.s=.o))
#C_FILES = $(wildcard *.c)
C_FILES = buffer.c clock.c idletask.c ipc.c list.c \
	mbox.c memory.c msgq.c mutex.c ptimer.c s_alloc.c \
	sem.c task.c taskq.c siprintf.c journal.c kprintf.c uprintf.c kservice.c net_task.c
C_OBJS = $(notdir $(C_FILES:.c=.o))

OBJS = $(S_OBJS) $(C_OBJS)

TARGET_LIB = $(LIB_PATH)/libkernel.a


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
	$(AR) rs $@ $?


