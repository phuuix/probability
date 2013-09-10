# config.mk for i386 bsp
#


TEXTBASE = 0x00000000

ifeq ($(TEXTBASE),)
	TEXTBASE = 0x0
endif

ifeq ($(LDSCRIPT),)
	LDSCRIPT = $(BOARD).lds
endif

#AFLAGS += -msoft-float -mapcs-32 -march=armv4 -mtune=arm7tdmi -DTEXT_BASE=$(TEXTBASE)
#CFLAGS += -msoft-float -mapcs-32 -march=armv4 -mtune=arm7tdmi -DTEXT_BASE=$(TEXTBASE)

AFLAGS += -mapcs -march=armv4 -mtune=arm7tdmi -DTEXT_BASE=$(TEXTBASE)
CFLAGS += -mapcs -march=armv4 -mtune=arm7tdmi -DTEXT_BASE=$(TEXTBASE)
LDFLAGS += -T $(LDSCRIPT) -Ttext $(TEXTBASE) -L$(LIB_PATH)


