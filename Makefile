CFLAGS=-lX11 -Os -ansi -pedantic -std=c99 -Wall -Werror -Werror=format=0 -fmax-errors=5
pede: pede.c atoms.o signal_events.o
	$(CC) $(CFLAGS) $^ -o $@
default: pede
clean:
	rm *.o pede || :
