IP ?= 192.168.4.1

.PHONY : all clean push

CFLAGS = -std=gnu99 -Wall -Wextra -O2 -march=native

all : execute_reflash page.mpfs push

mfsmaker : mfsmaker.c
	$(CC) $(CFLAGS) -o $@ $^

page.mpfs : mfsmaker page
#	cat to_compress/*.js | gzip -9 > page/compressed.js.gz
	./mfsmaker page page.mpfs

pushtodev : pushtodev.c
	$(CC) $(CFLAGS) -o $@ $^

execute_reflash : execute_reflash.c md5.c
	$(CC) $(CFLAGS) -o $@ $^

push : pushtodev page.mpfs
	./pushtodev $(IP) 65536 page.mpfs
#	./pushtodev $(IP) 1048576 page.mpfs

clean :
	$(RM) mfsmaker page.mpfs pushtodev execute_reflash
