CFLAGS=-lX11 -Os -ansi -pedantic -std=c99 -Wall -Werror -Werror=format=0 -fmax-errors=5
default: pede
util.o: data_sizes.h
pede: pede.c util.o wm_core.o atoms.o keys.o signal_events.o move_modifiers.o size_modifiers.o
	$(CC) $(CFLAGS) $^ -o $@
data_sizes.h: data_sizes
	./data_sizes > data_sizes.h
clean:
	rm *.o pede data_sizes data_sizes.h || :
