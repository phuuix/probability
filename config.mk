# Top level config.mk
#

# PROJ_ROOTDIR: Must define this environment variable in SHELL.
# BSP: Must define this environment variable in SHELL.

ifeq ($(BSP),)
BSP = stm32f2xxskm32
endif

# Defination for ep7312 bsp
ifeq ($(BSP), ep7312)
BOARD = ep7312
ARCH = ARM
CROSS_COMPILE = arm-elf-
endif

# Defination for i386 bsp
ifeq ($(BSP), i386)
BOARD = i386
ARCH = i386
CROSS_COMPILE = 
endif

# Defination for samsung wh44b0 bsp
ifeq ($(BSP), wh44b0)
BOARD = wh44b0
ARCH = ARM
CROSS_COMPILE = arm-elf-
endif

# Defination for NXP LPC2212/2214 bsp
ifeq ($(BSP), lpc221x)
BOARD = lpc221x
ARCH = ARM
CROSS_COMPILE = arm-none-eabi-
endif

# Defination for STM32F2xx bsp
ifeq ($(BSP), stm32f2xx)
BOARD = stm32f2xx
ARCH = ARM
CROSS_COMPILE = arm-none-eabi-
endif

# Defination for STM32F2xx SK-M32 bsp
ifeq ($(BSP), stm32f2xxskm32)
BOARD = stm32f2xxskm32
ARCH = ARM
CROSS_COMPILE = arm-none-eabi-
endif

# Definition for stm32f4 discovery bsp
ifeq ($(BSP), stm32f4discovery)
BOARD = stm32f4discovery
ARCH = ARM
CROSS_COMPILE = arm-none-eabi-
endif

AS  = $(CROSS_COMPILE)as
LD  = $(CROSS_COMPILE)ld
CC  = $(CROSS_COMPILE)gcc
CPP = $(CC) -E
AR  = $(CROSS_COMPILE)ar
NM  = $(CROSS_COMPILE)nm
STRIP   = $(CROSS_COMPILE)strip
OBJCOPY = $(CROSS_COMPILE)objcopy
OBJDUMP = $(CROSS_COMPILE)objdump
RANLIB  = $(CROSS_COMPILE)RANLIB

MKDIR	= mkdir
CP	= cp
RM	= rm -f

INCL   = . -I$(PROJ_ROOTDIR)/include -I$(PROJ_ROOTDIR)/libc/include -I$(PROJ_ROOTDIR)/bsp/$(BSP)
LIB_PATH    = $(PROJ_ROOTDIR)/lib

RELFLAGS = 
DBGFLAGS = #-DDEBUG
OPTFLAGS = #-Os -fomit-frame-pointer

CPPFLAGS = $(DBGFLAGS) $(OPTFLAGS) $(RELFLAGS) -nostdinc -fno-builtin -I$(INCL)

CFLAGS   := $(CPPFLAGS) -g -Wall -Wno-attributes -finline-functions -fno-stack-protector
#AFLAGS   := -E -D__ASSEMBLY__ $(CPPFLAGS)
AFLAGS   := -g -x assembler-with-cpp -D__ASSEMBLY__ $(CPPFLAGS)

LDFLAGS  = -nostdlib -Bstatic #-T $(LDSCRIPT) -Ttext $(TEXT_BASE) -L$(LIB_PATH)

include $(PROJ_ROOTDIR)/bsp/$(BOARD)/config.mk

# Common rules
%.o : %.s
	$(CC) $(AFLAGS) -c $<
%.o : %.S
	$(CC) $(AFLAGS) -c $<
%.o : %.c
	$(CC) $(CFLAGS) -c $<
