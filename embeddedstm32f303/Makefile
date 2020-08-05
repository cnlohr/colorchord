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

DEFS =  -DSTM32F30X 
STARTUP = lib/startup_stm32f30x.s

MCU = cortex-m4
MCFLAGS = -mcpu=$(MCU) -mthumb -mlittle-endian -mthumb-interwork

STM32_INCLUDES = -Ilib -I. -ISTM32F30x_StdPeriph_Driver/inc

OPTIMIZE       =  -Ofast  -mfloat-abi=hard -mfpu=fpv4-sp-d16 -flto -ffunction-sections -fdata-sections  -Wl,--relax 

CFLAGS	= $(MCFLAGS)  $(OPTIMIZE)  $(DEFS) \
	-I. \
	$(STM32_INCLUDES) \
	-I../embeddedcommon \
	 -Wl,-T,lib/stm32f303.ld

CFLAGS+=-DDEBUG

#If you use a TQFP STM32F303. (x6, x8)
CFLAGS+=-I. -DTQFP32  -DHSE_VALUE=8000000

#From the nucleos(sp?)  (STM32F303xB/xC)
#CFLAGS+=-I. -DHSE_VALUE=25000000

AFLAGS	= $(MCFLAGS) 

SRC = main.c \
	spi2812.c \
	stm32f30x_it.c \
	lib/system_stm32f30x.c \
	lib/systems.c \
	adc.c \
	STM32F30x_StdPeriph_Driver/src/stm32f30x_dma.c \
	STM32F30x_StdPeriph_Driver/src/stm32f30x_spi.c \
	STM32F30x_StdPeriph_Driver/src/stm32f30x_tim.c \
	STM32F30x_StdPeriph_Driver/src/stm32f30x_adc.c \
	STM32F30x_StdPeriph_Driver/src/stm32f30x_misc.c \
	STM32F30x_StdPeriph_Driver/src/stm32f30x_rcc.c \
	STM32F30x_StdPeriph_Driver/src/stm32f30x_gpio.c \
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
