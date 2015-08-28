FW_FILE_1:=0x00000.bin
FW_FILE_2:=0x40000.bin
TARGET_OUT:=image.elf
all : $(TARGET_OUT) $(FW_FILE_1) $(FW_FILE_2)


SRCS:=driver/uart.c \
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

GCC_FOLDER:=~/esp8266/esp-open-sdk/xtensa-lx106-elf
ESPTOOL_PY:=~/esp8266/esptool/esptool.py
FW_TOOL:=~/esp8266/other/esptool/esptool
SDK:=/home/cnlohr/esp8266/esp_iot_sdk_v1.3.0
PORT:=/dev/ttyUSB0
#PORT:=/dev/ttyACM0

XTLIB:=$(SDK)/lib
XTGCCLIB:=$(GCC_FOLDER)/lib/gcc/xtensa-lx106-elf/4.8.2/libgcc.a
FOLDERPREFIX:=$(GCC_FOLDER)/bin
PREFIX:=$(FOLDERPREFIX)/xtensa-lx106-elf-
CC:=$(PREFIX)gcc

CFLAGS:=-mlongcalls -I$(SDK)/include -Imyclib -Iinclude -Iuser -Os -I$(SDK)/include/ -I../embeddedcommon -I. -Icommon -DICACHE_FLASH

#-DFREQ==12500 seems to work well.


LDFLAGS_CORE:=\
	-nostdlib \
	-L$(XTLIB) \
	-L$(XTGCCLIB) \
	$(SDK)/lib/libmain.a \
	$(SDK)/lib/liblwip.a \
	$(SDK)/lib/libssl.a \
	$(SDK)/lib/libupgrade.a \
	$(SDK)/lib/libnet80211.a \
	$(SDK)/lib/libwpa.a \
	$(SDK)/lib/libnet80211.a \
	$(SDK)/lib/libphy.a \
	$(SDK)/lib/liblwip.a \
	$(SDK)/lib/libpp.a \
	$(SDK)/lib/libmain.a \
	$(XTGCCLIB) \
	-T $(SDK)/ld/eagle.app.v6.ld \
	-Wl,-Map,output.map \

#	-flto -Wl,--relax -Wl,--gc-sections


LINKFLAGS:= \
	$(LDFLAGS_CORE) \
	-B$(XTLIB)

#image.elf : $(OBJS)
#	$(PREFIX)ld $^ $(LDFLAGS) -o $@

$(TARGET_OUT) : $(SRCS)
	$(PREFIX)gcc $(CFLAGS) $^   $(LINKFLAGS) -o $@



$(FW_FILE_1): $(TARGET_OUT)
	@echo "FW $@"
	$(FW_TOOL) -eo $(TARGET_OUT) -bo $@ -bs .text -bs .data -bs .rodata -bc -ec

$(FW_FILE_2): $(TARGET_OUT)
	@echo "FW $@"
	$(FW_TOOL) -eo $(TARGET_OUT) -es .irom0.text $@ -ec

burn : $(FW_FILE_1) $(FW_FILE_2)
	($(ESPTOOL_PY) --port $(PORT) write_flash 0x00000 0x00000.bin 0x40000 0x40000.bin)||(true)


#If you have space, MFS should live at 0x100000, if you don't it can also live at
#0x10000.  But, then it is limited to 180kB.  You might need to do this if you have a 512kB 
#ESP variant.

burnweb : web/page.mpfs
	($(ESPTOOL_PY) --port $(PORT) write_flash 0x10000 web/page.mpfs)||(true)



IP?=192.168.4.1

netburn : image.elf $(FW_FILE_1) $(FW_FILE_2)
	web/execute_reflash $(IP) 0x00000.bin 0x40000.bin

clean :
	rm -rf user/*.o driver/*.o $(TARGET_OUT) $(FW_FILE_1) $(FW_FILE_2)


