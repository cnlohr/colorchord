PROJS:=test_using_square_wave_octave_approach test_using_work_selection_heap test_using_work_selection_table test_using_square_wave_octave_approach_stipple

all : $(PROJS)

CFLAGS:=-I../colorchord2/rawdraw -g
LDFLAGS:=-lGL -lm -lpthread -lX11

$(PROJS): %: %.c
	gcc -o $@ $^ $(CFLAGS) $(LDFLAGS)

clean :
	rm -rf $(PROJS)


