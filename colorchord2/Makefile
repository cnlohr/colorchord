all : colorchord

OUTS := OutputVoronoi.o DisplayArray.o OutputLinear.o DisplayPie.o DisplayNetwork.o DisplayUSB2812.o DisplayDMX.o OutputProminent.o RecorderPlugin.o DisplayHIDAPI.o hidapi.o OutputCells.o DisplaySHM.o DisplayFileWrite.o

WINGCC:= i686-w64-mingw32-gcc

WINGCCFLAGS:= -g -DICACHE_FLASH_ATTR= -I../embeddedcommon -Icnfa -Irawdraw -I. -O1   #-O2 -Wl,--relax -Wl,--gc-sections -ffunction-sections -fdata-sections
WINLDFLAGS:=-lwinmm -lgdi32 -lws2_32  -lsetupapi

RAWDRAWLIBS:=-lX11 -lm -lpthread -lXinerama -lXext
LDLIBS:=-lpthread -lasound -lm -lpulse-simple -lpulse -ludev -lrt


CFLAGS:=-g -O1 -flto -Wall -ffast-math -I../embeddedcommon -I.  -Icnfa -Irawdraw -DICACHE_FLASH_ATTR=
EXTRALIBS:=-lusb-1.0

OBJS:=main.o  dft.o decompose.o filter.o color.o notefinder.o util.o outdrivers.o $(OUTS) parameters.o chash.o hook.o ../embeddedcommon/DFT32.o configs.o

colorchord : $(OBJS)
	cc -o $@ $^ $(CFLAGS) $(LDLIBS) $(EXTRALIBS) $(RAWDRAWLIBS)


colorchord.exe : main.c  dft.c decompose.c filter.c color.c notefinder.c util.c outdrivers.c parameters.c chash.c OutputVoronoi.c OutputProminent.c DisplayArray.c OutputLinear.c DisplayPie.c DisplayNetwork.c hook.c RecorderPlugin.c ../embeddedcommon/DFT32.c OutputCells.c configs.c hidapi.c DisplayHIDAPI.c
	$(WINGCC) $(WINGCCFLAGS) -o $@ $^ $(WINLDFLAGS)


clean :
	rm -rf *.o *~ colorchord colorchord.exe embeddedcc $(OBJS)
