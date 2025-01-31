all : nctest

CFLAGS:=-Og -g -I../../colorchord2/rawdraw -I../../colorchord2/cnfa
LDFLAGS:=-lm -lX11 -lasound -lpulse -lGL

nctest : nctest.c
	gcc -o $@ $^ $(CFLAGS) $(LDFLAGS)

clean :
	rm -rf nctest
