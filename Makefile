all : colorchord

RAWDRAW:=DrawFunctions.o XDriver.o
SOUND:=sound.o sound_alsa.o sound_pulse.o sound_null.o
OUTS := OutputVoronoi.o DisplayArray.o OutputLinear.o DisplayPie.o DisplayNetwork.o DisplayUSB2812.o DisplayDMX.o OutputProminent.o

WINGCC:=i586-mingw32msvc-gcc
WINGCCFLAGS:= -O2 -Wl,--relax -Wl,--gc-sections -ffunction-sections -fdata-sections -s
WINLDFLAGS:=-lwinmm -lgdi32 -lws2_32 

RAWDRAWLIBS:=-lX11 -lm -lpthread -lXinerama -lXext
LDLIBS:=-lpthread -lasound -lm -lpulse-simple -lpulse
CFLAGS:=-g -Os -flto -Wall
EXTRALIBS:=-lusb-1.0

colorchord : os_generic.o main.o  dft.o decompose.o filter.o color.o sort.o notefinder.o util.o outdrivers.o $(RAWDRAW) $(SOUND) $(OUTS) parameters.o chash.o 
	gcc -o $@ $^ $(CFLAGS) $(LDLIBS) $(EXTRALIBS) $(RAWDRAWLIBS)

embeddedcc : os_generic.c embeddedcc.c dft.c embeddednf.c
	gcc -o $@ $^ $(CFLAGS) -DCCEMBEDDED $(LDFLAGS) $(EXTRALIBS) $(RAWDRAWLIBS)

runembedded : embeddedcc
	parec --format=u8 --rate=8000 --channels=1 --device=alsa_output.pci-0000_00_1b.0.analog-stereo.monitor | ./embeddedcc 


colorchord.exe : os_generic.c main.c  dft.c decompose.c filter.c color.c sort.c notefinder.c util.c outdrivers.c DrawFunctions.c parameters.c chash.c WinDriver.c sound.c sound_null.c sound_win.c OutputVoronoi.c DisplayArray.c OutputLinear.c DisplayPie.c DisplayNetwork.c
	$(WINGCC) $(WINGCCFLAGS) -o $@ $^ $(WINLDFLAGS)


clean :
	rm -rf *.o *~ colorchord colorchord.exe embeddedcc
