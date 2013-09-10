# config.mk for i386 bsp
#

TEXTBASE = 0x100000
LDSCRIPT = i386.lds

LDFLAGS += -T $(LDSCRIPT) -Ttext $(TEXTBASE) -L$(LIB_PATH)


