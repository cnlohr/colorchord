all : colorchord

RAWDRAW:=DrawFunctions.o XDriver.o
SOUND:=sound.o sound_alsa.o sound_pulse.o sound_null.o
OUTS := OutputVoronoi.o DisplayArray.o OutputLinear.o DisplayPie.o DisplayNetwork.o DisplayUSB2812.o DisplayDMX.o

WINGCC:=i586-mingw32msvc-gcc
WINGCCFLAGS:= -lwinmm -lgdi32 -lws2_32 -O2 -ffast-math -g

RAWDRAWLIBS:=-lX11 -lm -lpthread -lXinerama -lXext
LDLIBS:=-lpthread -lasound -lm -lpulse-simple -lpulse
CFLAGS:=-g -Os -flto -Wall -ffast-math
EXTRALIBS:=-lusb-1.0

colorchord : os_generic.o main.o  dft.o decompose.o filter.o color.o sort.o notefinder.o util.o outdrivers.o $(RAWDRAW) $(SOUND) $(OUTS) parameters.o chash.o 
	gcc -o $@ $^ $(CFLAGS) $(LDLIBS) $(EXTRALIBS) $(RAWDRAWLIBS)

colorchord.exe : os_generic.c main.c  dft.c decompose.c filter.c color.c sort.c notefinder.c util.c outdrivers.c DrawFunctions.c parameters.c chash.c WinDriver.c sound.c sound_null.c sound_win.c OutputVoronoi.c DisplayArray.c OutputLinear.c DisplayPie.c DisplayNetwork.c
	$(WINGCC) -o $@ $^ $(WINGCCFLAGS)


clean :
	rm -rf *.o *~ colorchord
