###############################################################################
# USER OPTIONS
###############################################################################

CHIP  ?= 8266
IP    ?= 192.168.4.1     # does not actually set the IP in firmware
PORT  ?= /dev/ttyUSB0    # could also be /dev/ttyACM0
PAGE_OFFSET ?= 65536     # 1048576

SDK_DEFAULT  ?= $(HOME)/esp8266/esp-open-sdk
ESP_GCC_VERS ?= 4.8.5

OPTS += -DICACHE_FLASH
#OPTS += -DFREQ=12500

###########################################################################VARS

GCC_FOLDER = $(ESP_ROOT)/xtensa-lx106-elf
ESPTOOL_PY = $(ESP_ROOT)/esptool/esptool.py
SDK        = $(ESP_ROOT)/sdk

XTLIB        = $(SDK)/lib
XTGCCLIB     = $(GCC_FOLDER)/lib/gcc/xtensa-lx106-elf/$(ESP_GCC_VERS)/libgcc.a
FOLDERPREFIX = $(GCC_FOLDER)/bin
PREFIX       = $(FOLDERPREFIX)/xtensa-lx106-elf-
CC           = $(PREFIX)gcc
LD           = $(PREFIX)ld
AR           = $(PREFIX)ar

ESP_ROOT := $(shell echo "$$ESP_ROOT")
ifeq ($(strip $(ESP_ROOT)),)
$(warning Warning: No shell variable 'ESP_ROOT', using '$(SDK_DEFAULT)')
ESP_ROOT := $(SDK_DEFAULT)
endif
