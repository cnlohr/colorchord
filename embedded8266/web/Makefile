include ../makeconf.inc   # Look here for user configuration

.PHONY : all clean push
all : execute_reflash push

CC = gcc

mfsmaker : mfsmaker.c
pushtodev : pushtodev.c
execute_reflash : execute_reflash.c md5.c

page.mpfs : mfsmaker page
#	cat to_compress/*.js | gzip -9 > page/compressed.js.gz
	./mfsmaker page page.mpfs

#mfsmaker pushtodev execute_reflash:
#	$(CC) $(CFLAGS) -o $@ $^

push : pushtodev page.mpfs
	./pushtodev $(IP) $(PAGE_OFFSET) page.mpfs

clean :
	$(RM) mfsmaker page.mpfs pushtodev execute_reflash
