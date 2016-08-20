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


#Useful git commands
ifndef TARGET
# Fetch submodule if the user forgot to clone with `--recursive`
GETSUBMODS = all burn burnweb netweb netburn clean cleanall purge
.PHONY : $(GETSUBMODS)
$(GETSUBMODS) :
    $(warning Submodule esp82xx was not fetched. Trying it now.)
    git submodule update --init --recursive
    $(info Re-unning make...)
    make $@ $(MFLAGS) $(MAKEOVERRIDES)
endif
