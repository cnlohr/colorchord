all : composer

CFLAGS:=-DOCTAVES=6 -DFIXBPERO=48

CX := ../colorchord2
CM := ../embeddedcommon

OUTS := $(CX)/OutputVoronoi.o $(CX)/DisplayArray.o $(CX)/OutputLinear.o $(CX)/DisplayPie.o $(CX)/DisplayNetwork.o $(CX)/DisplayUSB2812.o $(CX)/DisplayDMX.o $(CX)/OutputProminent.o $(CX)/RecorderPlugin.o $(CX)/OutputCells.o

COMMON := $(OUTS) $(CX)/chash.o $(CX)/color.o $(CX)/decompose.o $(CX)/dft.o $(CX)/filter.o $(CX)/DrawFunctions.o $(CX)/hook.o $(CX)/notefinder.o $(CX)/os_generic.o $(CX)/parameters.o $(CX)/sort.o  $(CX)/sound_null.o  $(CX)/sound.o  $(CX)/sound_pulse.o $(CX)/util.o  $(CM)/DFT32.o $(CX)/configs.o $(CX)/outdrivers.o

LINUX   := $(COMMON) $(CX)/XDriver.o $(CX)/sound_alsa.o
WINDOWS := $(COMMON) $(CX)/WinDriver.o $(CX)/sound_win.o


WINGCCFLAGS:= -g -DICACHE_FLASH_ATTR= -I../embeddedcommon -I. -O0   #-O2 -Wl,--relax -Wl,--gc-sections -ffunction-sections -fdata-sections
WINLDFLAGS:=-lwinmm -lgdi32 -lws2_32

CFLAGS:=$(CFLAGS) -I$(CX) -I../embeddedcommon -DICACHE_FLASH_ATTR=
#CFLAGS:=-flto -Os -ffast-math -Wl,--relax -Wl,--gc-sections -ffunction-sections -fdata-sections    
CFLAGS:=$(CFLAGS) -g -O2
LDFLAGS:=-lm -lX11 -lpulse -lasound -lpthread -lXinerama -lusb-1.0 -lXext -g

composer : composer.o $(LINUX)
	gcc -o $@ $^ $(LDFLAGS)

clean :
	rm -rf *.o *~ $(LINUX) $(WINDOWS)

