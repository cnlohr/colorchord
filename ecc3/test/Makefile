all : test

CFLAGS := -I../../colorchord2/rawdraw

test : test.c
	gcc $(CFLAGS) -o $@ $^ -lX11 -lpulse

clean :
	rm -rf *.o *~

