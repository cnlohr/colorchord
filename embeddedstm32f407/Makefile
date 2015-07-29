TARGET=main
all: burn

PREFIX=arm-none-eabi

CC=$(PREFIX)-gcc
LD=$(PREFIX)-gcc
AS=$(PREFIX)-as
CP=$(PREFIX)-objcopy
OD=$(PREFIX)-objdump


OBJCOPYFLAGS = -O binary

BIN=$(CP) -O ihex 

DEFS =  -DSTM32F40_41xxx  -DHSE_VALUE=8000000
STARTUP = lib/startup_stm32f40_41xxx.s
STLIB = STM32F4xx_StdPeriph_Driver
MCU = cortex-m3
MCFLAGS = -mcpu=$(MCU) -mthumb -mlittle-endian -mthumb-interwork

STM32_INCLUDES = -Ilib -I. -I$(STLIB)/inc


OPTIMIZE       = -Os

CFLAGS	= $(MCFLAGS)  $(OPTIMIZE)  $(DEFS) \
	-I. \
	$(STM32_INCLUDES) \
	-I../embeddedcommon \
	-Wl,-T,lib/stm32f407.ld

CFLAGS+=-DDEBUG
CFLAGS+=-I.

AFLAGS	= $(MCFLAGS) 

SRC = main.c \
	stm32f4xx_it.c \
	lib/system_stm32f4xx.c \
	lib/systems.c \
	spi2812.c \
	mp45dt02.c \
	lib/libPDMFilter_GCC.a \
	$(STLIB)/src/misc.c \
	$(STLIB)/src/stm32f4xx_dma.c \
	$(STLIB)/src/stm32f4xx_spi.c \
	$(STLIB)/src/stm32f4xx_syscfg.c \
	$(STLIB)/src/stm32f4xx_exti.c \
	$(STLIB)/src/stm32f4xx_rcc.c \
	$(STLIB)/src/stm32f4xx_gpio.c \
	../embeddedcommon/DFT32.c \
	../embeddedcommon/embeddednf.c \
	../embeddedcommon/embeddedout.c


burn : $(TARGET).bin
	openocd -f flash.cfg #-d3
terminal :
	openocd -f terminal.cfg

$(TARGET).bin : $(TARGET).out
	$(CP) $(OBJCOPYFLAGS) $< $@

$(TARGET).hex: $(EXECUTABLE)
	$(CP) -O ihex $^ $@

$(TARGET).out : $(SRC) $(STARTUP)
	$(CC) $(CFLAGS) $^ -lm -lc -lnosys  -o $@

clean:
	rm -f $(TARGET).lst $(TARGET).out $(TARGET).hex $(TARGET).bin $(TARGET).map  $(EXECUTABLE)
