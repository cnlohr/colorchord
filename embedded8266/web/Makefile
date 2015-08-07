all : execute_reflash page.dat push 

IP?=192.168.4.1

mfsmaker : mfsmaker.c
	gcc -o $@ $^

page.dat : mfsmaker page
#	cat to_compress/*.js | gzip -9 > page/compressed.js.gz
	./mfsmaker page page.dat

pushtodev : pushtodev.c
	gcc -o $@ $^

execute_reflash : execute_reflash.c md5.c
	gcc -o $@ $^

push : pushtodev page.dat
	./pushtodev $(IP) 1048576 page.dat

clean :
	rm -rf mfsmaker page.dat pushtodev execute_reflash
