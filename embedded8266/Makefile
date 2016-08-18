include user.cfg
include esp82xx/common.mf
include esp82xx/main.mf

SRCS += user/ws2812_i2s.c \
	user/hpatimer.c \
	user/adc.c \
	../embeddedcommon/DFT32.c \
	../embeddedcommon/embeddednf.c \
	../embeddedcommon/embeddedout.c

LDFLAGS_CORE += -Wl,-Map,output.map

