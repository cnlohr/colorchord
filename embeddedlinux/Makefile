all : embeddedcc

CFLAGS:=-Ofast -DCCEMBEDDED -I.. -flto -m32 -DDFREQ=11025 -I../embeddedcommon -DNUM_LIN_LEDS=20
LDFLAGS:=-ffunction-sections -Wl,--gc-sections -fno-asynchronous-unwind-tables -Wl,--strip-all

embeddedcc :  ../embeddedcommon/embeddednf.c ../embeddedcommon/DFT32.c embeddedcc.c  ../embeddedcommon/embeddedout.c
	gcc -o $@ $^ $(CFLAGS) $(LDFLAGS)

runembedded : embeddedcc
	parec --format=u8 --rate=11025 --channels=1 --device=alsa_output.pci-0000_00_1b.0.analog-stereo.monitor --latency=128 | ./embeddedcc 192.168.4.1 0

clean :
	rm -rf embeddedcc *~
