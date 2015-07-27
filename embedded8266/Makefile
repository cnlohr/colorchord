all : image.elf
FW_FILE_1:=0x00000.bin
FW_FILE_2:=0x40000.bin

TARGET_OUT:=image.elf


SRCS:=driver/uart.c \
	user/mystuff.c \
	user/ws2812_i2s.c \
	user/hpatimer.c \
	driver/adc.c \
	../embeddedcommon/DFT32.c \
	../embeddedcommon/embeddednf.c \
	../embeddedcommon/embeddedout.c \
	user/user_main.c 

GCC_FOLDER:=~/esp8266/esp-open-sdk/xtensa-lx106-elf
ESPTOOL_PY:=~/esp8266/esptool/esptool.py
FW_TOOL:=~/esp8266/other/esptool/esptool
SDK:=/home/cnlohr/esp8266/esp_iot_sdk_v1.2.0
PORT:=/dev/ttyUSB0
#PORT:=/dev/ttyACM0

XTLIB:=$(SDK)/lib
XTGCCLIB:=$(GCC_FOLDER)/lib/gcc/xtensa-lx106-elf/4.8.2/libgcc.a
FOLDERPREFIX:=$(GCC_FOLDER)/bin
PREFIX:=$(FOLDERPREFIX)/xtensa-lx106-elf-
CC:=$(PREFIX)gcc

CFLAGS:=-mlongcalls -I$(SDK)/include -Imyclib -Iinclude -Iuser -Os -I$(SDK)/include/ -I../embeddedcommon -Dmemcpy=ets_memcpy -Dmemset=ets_memset \
	 -DDFREQ=16000 -DCCEMBEDDED -DNUM_LIN_LEDS=24
#-DFREQ==12500 seems to work well.


LDFLAGS_CORE:=\
	-nostdlib \
	-Wl,--relax -Wl,--gc-sections \
	-L$(XTLIB) \
	-L$(XTGCCLIB) \
	$(SDK)/lib/liblwip.a \
	$(SDK)/lib/libssl.a \
	$(SDK)/lib/libupgrade.a \
	$(SDK)/lib/libnet80211.a \
	$(SDK)/lib/liblwip.a \
	$(SDK)/lib/libwpa.a \
	$(SDK)/lib/libnet80211.a \
	$(SDK)/lib/libphy.a \
	$(SDK)/lib/libmain.a \
	$(SDK)/lib/libpp.a \
	$(XTGCCLIB) \
	-T $(SDK)/ld/eagle.app.v6.ld

LINKFLAGS:= \
	$(LDFLAGS_CORE) \
	-B$(XTLIB)

#image.elf : $(OBJS)
#	$(PREFIX)ld $^ $(LDFLAGS) -o $@

$(TARGET_OUT) : $(SRCS)
	$(PREFIX)gcc $(CFLAGS) $^  -flto $(LINKFLAGS) -o $@



$(FW_FILE_1): $(TARGET_OUT)
	@echo "FW $@"
	$(FW_TOOL) -eo $(TARGET_OUT) -bo $@ -bs .text -bs .data -bs .rodata -bc -ec

$(FW_FILE_2): $(TARGET_OUT)
	@echo "FW $@"
	$(FW_TOOL) -eo $(TARGET_OUT) -es .irom0.text $@ -ec

burn : $(FW_FILE_1) $(FW_FILE_2)
	($(ESPTOOL_PY) --port $(PORT) write_flash 0x00000 0x00000.bin 0x40000 0x40000.bin)||(true)


clean :
	rm -rf user/*.o driver/*.o $(TARGET_OUT) $(FW_FILE_1) $(FW_FILE_2)


