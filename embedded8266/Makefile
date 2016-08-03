###############################################################################
# USER OPTIONS
###############################################################################

CHIP  ?= 8266
IP    ?= 192.168.4.1
PORT  ?= /dev/ttyUSB0
#PORT ?= /dev/ttyACM0

ESP_ROOT ?= $(HOME)/esp8266/esp-open-sdk
GCC_VERS ?= 4.8.5

OPTS += -DICACHE_FLASH
#OPTS += -DFREQ=12500

###########################################################################VARS

.PHONY : all clean cleanall netburn burnweb burn
uniq = $(if $1,$(firstword $1) $(call uniq,$(filter-out $(firstword $1),$1)))

FW_FILE_1 = 0x00000.bin
FW_FILE_2 = 0x40000.bin
TARGET = image.elf

SRCS = \
	driver/uart.c \
	common/mystuff.c \
	common/flash_rewriter.c \
	common/http.c \
	common/http_custom.c \
	common/mfs.c \
	user/ws2812_i2s.c \
	user/hpatimer.c \
	user/custom_commands.c \
	driver/adc.c \
	../embeddedcommon/DFT32.c \
	../embeddedcommon/embeddednf.c \
	../embeddedcommon/embeddedout.c \
	user/user_main.c  \
	common/commonservices.c

LIBS = main lwip ssl upgrade net80211 wpa net80211 phy lwip pp crypto
INCL = $(SDK)/include myclib include .

GCC_FOLDER = $(ESP_ROOT)/xtensa-lx106-elf
ESPTOOL_PY = $(ESP_ROOT)/esptool/esptool.py
FW_TOOL    = $(ESP_ROOT)/../esptool-ck/esptool
SDK        = $(ESP_ROOT)/sdk

XTLIB        = $(SDK)/lib
XTGCCLIB     = $(GCC_FOLDER)/lib/gcc/xtensa-lx106-elf/$(GCC_VERS)/libgcc.a
PREFIX       = $(GCC_FOLDER)/bin/xtensa-lx106-elf-
CC           = $(PREFIX)gcc
LD           = $(PREFIX)ld

CFLAGS = -mlongcalls -Os $(addprefix -I,$(INCL) $(call uniq, $(patsubst %/,%,$(dir $(SRCS))))) $(OPTS)

LDFLAGS_CORE = -nostdlib -L$(XTLIB) -L$(XTGCCLIB) \
	$(addprefix $(SDK)/lib/lib,$(addsuffix .a,$(LIBS))) \
	$(XTGCCLIB) -T $(SDK)/ld/eagle.app.v6.ld -Wl,-Map,output.map
#	-flto -Wl,--relax -Wl,--gc-sections

LINKFLAGS = $(LDFLAGS_CORE) -B$(XTLIB)

##########################################################################RULES

all : $(FW_FILE_1) $(FW_FILE_2)

$(FW_FILE_1): $(TARGET)
	@echo "FW $@"
	$(FW_TOOL) -eo $(TARGET) -bo $@ -bs .text -bs .data -bs .rodata -bc -ec

$(FW_FILE_2): $(TARGET)
	@echo "FW $@"
	$(FW_TOOL) -eo $(TARGET) -es .irom0.text $@ -ec

$(TARGET) : $(SRCS)
	$(CC) $(CFLAGS) $^   $(LINKFLAGS) -o $@
#	$(LD) $^ $(LDFLAGS) -o $@

ifeq ($(CHIP), 8285)
burn : $(FW_FILE_1) $(FW_FILE_2)
	($(ESPTOOL_PY) --port $(PORT) write_flash -fs 8m -fm dout 0x00000 0x00000.bin 0x40000 0x40000.bin)||(true)
else ifeq ($(CHIP), 8266)
burn : $(FW_FILE_1) $(FW_FILE_2)
	($(ESPTOOL_PY) --port $(PORT) write_flash 0x00000 0x00000.bin 0x40000 0x40000.bin)||(true)
else
	$(error Error: Unknown chip '$(CHIP)')
endif

#If you have space, MFS should live at 0x100000, if you don't it can also live at
#0x10000.  But, then it is limited to 180kB.  You might need to do this if you have a 512kB
#ESP variant.

burnweb : web/page.mpfs
	($(ESPTOOL_PY) --port $(PORT) write_flash 0x10000 web/page.mpfs)||(true)

netburn : image.elf $(FW_FILE_1) $(FW_FILE_2)
	web/execute_reflash $(IP) 0x00000.bin 0x40000.bin

clean :
	$(RM) $(patsubst %.c,%.o,$(SRCS)) $(TARGET)

purge : clean
	$(RM) $(FW_FILE_1) $(FW_FILE_2)
