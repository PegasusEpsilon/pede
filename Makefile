WARNINGS=-Wall -Wextra -Wshadow -Wconversion -Werror -Werror=format=0
FLAGS=-fmax-errors=0
LIBS=-lX11
CFLAGS=-Os -ansi -pedantic -std=c99 $(WARNINGS) $(FLAGS) $(LIBS)
MODULES=\
	util.o pager.o wm_core.o atoms.o keys.o signal_events.o move_modifiers.o \
	size_modifiers.o
default: pede
pede: pede.c $(MODULES)
	$(CC) $(CFLAGS) $^ -o $@
	strip pede
clean:
	rm *.o pede || :
