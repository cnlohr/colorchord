all : embeddedcc

CFLAGS:=-Ofast -DCCEMBEDDED -I.. -flto -m32
LDFLAGS:=-ffunction-sections -Wl,--gc-sections -fno-asynchronous-unwind-tables -Wl,--strip-all

embeddedcc :  ../embeddednf.c ../DFT32.c embeddedcc.c 
	gcc -o $@ $^ $(CFLAGS) $(LDFLAGS)

runembedded : embeddedcc
	parec --format=u8 --rate=8000 --channels=1 --device=alsa_output.pci-0000_00_1b.0.analog-stereo.monitor | ./embeddedcc 

clean :
	rm -rf embeddedcc *~
