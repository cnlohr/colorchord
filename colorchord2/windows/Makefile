all : ../colorchord.exe

OUTS := OutputVoronoi.o DisplayArray.o OutputLinear.o DisplayPie.o DisplayNetwork.o DisplayUSB2812.o DisplayDMX.o OutputProminent.o RecorderPlugin.o DisplayHIDAPI.o hidapi.o OutputCells.o DisplaySHM.o DisplayFileWrite.o

SRCS := ../main.c  ../dft.c ../decompose.c ../filter.c ../color.c ../notefinder.c ../util.c ../outdrivers.c 
SRCS += ../parameters.c ../chash.c ../OutputVoronoi.c ../OutputProminent.c ../DisplayArray.c 
SRCS += ../OutputLinear.c ../DisplayPie.c ../DisplayNetwork.c ../hook.c ../RecorderPlugin.c 
SRCS += ../../embeddedcommon/DFT32.c ../OutputCells.c ../configs.c ../hidapi.c ../DisplayHIDAPI.c

WINGCC:= clang -fcolor-diagnostics

WINGCCFLAGS:= -g -D_CRT_SECURE_NO_WARNINGS -Wno-deprecated-declarations -DICACHE_FLASH_ATTR= -I../../embeddedcommon -I../cnfa -I../rawdraw -I../ -O1   #-O2 -Wl,--relax -Wl,--gc-sections -ffunction-sections -fdata-sections
WINLDFLAGS:=-lwinmm -lgdi32 -lws2_32 -lsetupapi -lkernel32 -luser32 -ldbghelp -lole32 -lmmdevapi -lAvrt

RAWDRAWLIBS:=-lX11 -lm -lpthread -lXinerama -lXext
LDLIBS:=-lpthread -lasound -lm -lpulse-simple -lpulse -ludev -lrt

OBJS:=../main.o ../dft.o ../decompose.o ../filter.o ../color.o ../notefinder.o ../util.o ../outdrivers.o $(OUTS) ../parameters.o ../chash.o ../hook.o ../../embeddedcommon/DFT32.o ../configs.o

../colorchord.exe :  $(SRCS)
	$(WINGCC) $(WINGCCFLAGS) -o $@ $^ $(WINLDFLAGS)

clean :
	rm -rf *.o *~ ../colorchord ../colorchord.exe ../embeddedcc $(OBJS)
