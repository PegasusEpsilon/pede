WARNINGS = -Wall -Wextra -Wshadow -Wconversion -Winline -Werror

ifeq ($(shell $(CC) --version | awk 'NR == 1 { print $$1 }'),clang)
ifeq ($(SHIFTCHECK),yes)
$(info $(shell tput setaf 9; tput bold))
$(info ### ATTENTION ### SHIFTCHECK is for testing, not for actual use)
$(info The output binary will be absolutely massive compared to normal)
$(info $(shell tput sgr0))
FLAGS = -fsanitize=shift
endif
else
FLAGS = -fmax-errors=0
WARNINGS += -Werror=format=0
endif

CFLAGS = -Os -ansi -pedantic -std=c99 $(WARNINGS) $(FLAGS)
MODULES = \
	util.o pager.o wm_core.o atoms.o keys.o signal_events.o move_modifiers.o \
	size_modifiers.o

R := `tput setaf 9`
G := `tput setaf 10`
Y := `tput setaf 11`
W := `tput setaf 15`
RST := `tput sgr0`

default: pede

.c.o:
	@echo $W[$Ycompile:$(CC)$W]$G $@$(RST)
	@$(CC) $(CFLAGS) -c $^ -o $@

pede: pede.c $(MODULES)
	@echo $W[$Ylink:$(CC)$W]$G $@$(RST)
	@$(CC) $(CFLAGS) $^ -lX11 -o $@
	@echo $W[$Ystrip$W]$G $@$(RST)
	@strip -s pede

clean:
	@echo $(W)[$(R)clean$(W)]$(RST)
	@rm *.o pede || :
