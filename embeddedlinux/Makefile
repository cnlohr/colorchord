all : embeddedcc dummy_leds

CFLAGS:=-Os -I. -I.. -flto -I../embeddedcommon 

LDFLAGS:= -s -Wl,--relax -Wl,-Map=test.map  -Wl,--gc-sections -ffunction-sections -fdata-sections -Wl,--strip-all -fno-asynchronous-unwind-tables
#LDFLAGS:=-O0 -g


embeddedcc :  ../embeddedcommon/embeddednf.c ../embeddedcommon/DFT32.c embeddedcc.c  ../embeddedcommon/embeddedout.c
	gcc -o $@ $^ $(CFLAGS) $(LDFLAGS)

dummy_leds : dummy_leds.c 
	gcc -o $@ $^ -lX11 -lpthread $(CFLAGS) $(LDFLAGS)

SOUNDDEVICE:= --device=alsa_output.pci-0000_00_1b.0.analog-stereo.monitor 
#OUTIP:=192.168.4.1
OUTIP:=127.0.0.1

runembedded : embeddedcc
	parec --format=u8 --rate=16000 --channels=1 $(SOUNDDEVICE) --latency=128 | ./embeddedcc $(OUTIP) 1


clean :
	rm -rf embeddedcc *~ dummy_leds
