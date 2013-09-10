# config.mk for stm32f2xx bsp
#


TEXTBASE = 0x00000000
#LDSCRIPT = stm32_flash.ld

ifeq ($(TEXTBASE),)
	TEXTBASE = 0x0
endif

ifeq ($(LDSCRIPT),)
	LDSCRIPT = $(BOARD).lds
endif

# -mapcs has no effect, why?
AFLAGS += -mapcs -mcpu=cortex-m3 -mthumb -DTEXT_BASE=$(TEXTBASE)
CFLAGS += -mapcs -mcpu=cortex-m3 -mthumb -DTEXT_BASE=$(TEXTBASE)
#LDFLAGS += -T $(LDSCRIPT) -Ttext $(TEXTBASE) -L$(LIB_PATH)
LDFLAGS += -T $(LDSCRIPT) -L$(LIB_PATH)


