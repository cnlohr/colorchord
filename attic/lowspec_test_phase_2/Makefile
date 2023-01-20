PROJS:=basic_oldstyle_test

all : $(PROJS)

CFLAGS:=-I../../colorchord2/rawdraw -g
LDFLAGS:=-lGL -lm -lpthread -lX11

$(PROJS): %: %.c
	gcc -o $@ $^ $(CFLAGS) $(LDFLAGS)

clean :
	rm -rf $(PROJS)


