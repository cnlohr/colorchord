all : embeddedcc dummy_leds

CFLAGS:=-Os -I. -I.. -flto -I../embeddedcommon 

LDFLAGS:= -s -Wl,--relax -Wl,-Map=test.map  -Wl,--gc-sections -ffunction-sections -fdata-sections -Wl,--strip-all -fno-asynchronous-unwind-tables
#LDFLAGS:=-O0 -g


embeddedcc :  ../embeddedcommon/embeddednf.c ../embeddedcommon/DFT32.c embeddedcc.c  ../embeddedcommon/embeddedout.c
	gcc -o $@ $^ $(CFLAGS) $(LDFLAGS) -m32

dummy_leds : dummy_leds.c 
	gcc -o $@ $^ -lX11 -lpthread $(CFLAGS) $(LDFLAGS)


#If specifiy device not seems to work
#To find correct devices looked here and used pacmd and list-cards
#https://ubuntuforums.org/showthread.php?t=1517726
#SOUNDDEVICE:= --device=alsa_output.pci-0000_00_05.0.analog-stereo.monitor # to use playing of mp3
SOUNDDEVICE:= --device=alsa_input.pci-0000_00_05.0.analog-stereo # to use mic
#By leading this blank it takes a default output which works 
#SOUNDDEVICE:= 

#OUTIP:=192.168.20.9
#OUTIP:=192.168.1.5 # not work for ubuntu guest in Virtual Machine
#OUTIP:=10.0.2.15 # works for ubuntu guest in Virtual Machine
OUTIP:=127.0.0.1 # works for ubuntu guest in Virtual Machine

runembedded : embeddedcc
	parec  --format=u8 --rate=16000 --channels=1 $(SOUNDDEVICE) --latency=128 | ./embeddedcc $(OUTIP) 1 

runembeddeddbg : embeddedcc
	parec -v  --format=u8 --rate=16000 --channels=1 $(SOUNDDEVICE) --latency=128 | ./embeddedcc $(OUTIP) 1 > dbout.txt

clean :
	rm -rf embeddedcc *~ dummy_leds
