all : test

CFLAGS := -I../../colorchord2/rawdraw

test : test.c
	gcc $(CFLAGS) -o $@ $^ -lX11 -lpulse -lm

clean :
	rm -rf *.o *~ test

