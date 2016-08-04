include makeconf.inc   # Look here for user configuration

.PHONY : all clean cleanall netburn burnweb burn
uniq = $(if $1,$(firstword $1) $(call uniq,$(filter-out $(firstword $1),$1)))

FW_FILE1 = image.elf-0x00000.bin
FW_FILE2 = image.elf-0x40000.bin
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

CFLAGS = -mlongcalls -Os $(addprefix -I,$(INCL) $(call uniq, $(patsubst %/,%,$(dir $(SRCS))))) $(OPTS)

LDFLAGS_CORE = -nostdlib -L$(XTLIB) -L$(XTGCCLIB) \
	$(addprefix $(SDK)/lib/lib,$(addsuffix .a,$(LIBS))) \
	$(XTGCCLIB) -T $(SDK)/ld/eagle.app.v6.ld -Wl,-Map,output.map
#	-flto -Wl,--relax -Wl,--gc-sections

LINKFLAGS = $(LDFLAGS_CORE) -B$(XTLIB)

##########################################################################RULES

all : $(FW_FILE1) $(FW_FILE2)

$(FW_FILE1) $(FW_FILE2) : $(TARGET)
	@echo "FW $@"
	export PATH="$(FOLDERPREFIX):$$PATH"; $(ESPTOOL_PY) elf2image $(TARGET)

$(TARGET) : $(SRCS)
	$(CC) $(CFLAGS) $^   $(LINKFLAGS) -o $@

ifeq ($(CHIP), 8285)
burn : $(FW_FILE1) $(FW_FILE2)
	($(ESPTOOL_PY) --port $(PORT) write_flash -fs 8m -fm dout 0x00000 0x00000.bin 0x40000 0x40000.bin)||(true)
else ifeq ($(CHIP), 8266)
burn : $(FW_FILE1) $(FW_FILE2)
	($(ESPTOOL_PY) --port $(PORT) write_flash 0x00000 $(FW_FILE1) 0x40000 $(FW_FILE2))||(true)
else
	$(error Error: Unknown chip '$(CHIP)')
endif

burnweb :
	@cd web && $(MAKE) $(MFLAGS) $(MAKEOVERRIDES) page.mpfs
	($(ESPTOOL_PY) --port $(PORT) write_flash 0x10000 web/page.mpfs)||(true)
#If you have space, MFS should live at 0x100000. It can also live at
#0x10000. But, then it is limited to 180kB. You might need to do this if
# you have a 512kB ESP variant.

netburn : $(FW_FILE1) $(FW_FILE2)
	@cd web && $(MAKE) $(MFLAGS) $(MAKEOVERRIDES) execute_reflash
	web/execute_reflash $(IP) $(FW_FILE1) $(FW_FILE2)

clean :
	$(RM) $(patsubst %.c,%.o,$(SRCS)) $(TARGET)

purge : clean
	@cd web && $(MAKE) $(MFLAGS) $(MAKEOVERRIDES) clean
	$(RM) $(FW_FILE1) $(FW_FILE2)

