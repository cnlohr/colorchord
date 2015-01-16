all : colorchord

RAWDRAW:=DrawFunctions.o XDriver.o
SOUND:=sound.o sound_alsa.o sound_pulse.o sound_null.o
OUTS := OutputVoronoi.o DisplayArray.o OutputLinear.o DisplayPie.o DisplayNetwork.o

#LEDOUTDriver.o DisplayOUTDriver.o

RAWDRAWLIBS:=-lX11 -lm -lpthread -lXinerama -lXext
LDLIBS:=-lpthread -lasound -lm -lpulse-simple -lpulse
CFLAGS:=-g -Os -flto -Wall -ffast-math
EXTRALIBS:=-lusb-1.0

colorchord : os_generic.o main.o  dft.o decompose.o filter.o color.o sort.o notefinder.o util.o outdrivers.o $(RAWDRAW) $(SOUND) $(OUTS) parameters.o chash.o 
	gcc -o $@ $^ $(CFLAGS) $(LDLIBS) $(EXTRALIBS) $(RAWDRAWLIBS)


clean :
	rm -rf *.o *~ colorchord
