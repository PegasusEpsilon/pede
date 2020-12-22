CFLAGS=-lX11 -Os -ansi -pedantic -std=c99 -Wall -Werror -Werror=format=0 -fmax-errors=5
default: pede
pede: pede.c util.o wm_core.o atoms.o keys.o signal_events.o move_modifiers.o size_modifiers.o
	$(CC) $(CFLAGS) $^ -o $@
clean:
	rm *.o pede || :
