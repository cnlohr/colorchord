all : execute_reflash page.dat push 

mfsmaker : mfsmaker.c
	gcc -o $@ $^

page.dat : mfsmaker page
	./mfsmaker page page.dat

pushtodev : pushtodev.c
	gcc -o $@ $^

execute_reflash : execute_reflash.c md5.c
	gcc -o $@ $^

push : pushtodev page.dat
	./pushtodev 192.168.4.1 1048576 page.dat

clean :
	rm -rf mfsmaker page.dat pushtodev execute_reflash
