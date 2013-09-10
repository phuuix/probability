# config.mk for hostsim bsp
#


TEXTBASE = 0x00000000

ifeq ($(TEXTBASE),)
	TEXTBASE = 0x0
endif

ifeq ($(LDSCRIPT),)
	LDSCRIPT = $(BOARD).lds
endif

# -mapcs has no effect, why?
LDFLAGS += -L$(LIB_PATH)


