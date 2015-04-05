all : embeddedcc

CFLAGS:=-Ofast -DCCEMBEDDED -I.. -flto -m32 -DDFREQ=11025
LDFLAGS:=-ffunction-sections -Wl,--gc-sections -fno-asynchronous-unwind-tables -Wl,--strip-all

embeddedcc :  ../embeddednf.c ../DFT32.c embeddedcc.c  ../embeddedout.c
	gcc -o $@ $^ $(CFLAGS) $(LDFLAGS)

runembedded : embeddedcc
	parec --format=u8 --rate=11025 --channels=1 --device=alsa_output.pci-0000_00_1b.0.analog-stereo.monitor --latency=128 | ./embeddedcc 

clean :
	rm -rf embeddedcc *~
