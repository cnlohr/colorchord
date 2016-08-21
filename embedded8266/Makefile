include user.cfg
-include esp82xx/common.mf
-include esp82xx/main.mf

% :
	$(warning This is the empty rule. Something went wrong.)
	@true

SRCS += user/ws2812_i2s.c \
	user/hpatimer.c \
	user/adc.c \
	../embeddedcommon/DFT32.c \
	../embeddedcommon/embeddednf.c \
	../embeddedcommon/embeddedout.c

LDFLAGS_CORE += -Wl,-Map,output.map


ifndef TARGET
$(info Modules were not checked out... use git clone --recursive in the future. Pulling now.)
$(shell git submodule update --init --recursive)
endif

