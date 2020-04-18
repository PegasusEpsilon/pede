/* signal_events.c - Pegasus Epsilon's Desktop Environment
** Goal: Less than a megabyte resident
** (C)opyright 2019-2020 "Pegasus Epsilon" <pegasus@pimpninjas.org>
** Distribute Unmodified - https://pegasus.pimpninjas.org/license
*/

#define _POSIX_C_SOURCE 200112L	// struct sigaction

#include <signal.h>	// struct sigaction, sigemptyset(), sigaction(), SIG(USR1, USR2, INT)
#include <stdio.h>	// puts(), printf(), perror(), fflush(), stdout
#include <stdlib.h>	// exit()

int which_signal = 0;

const char *signame (const int s) {
	switch (s) {
#define SIGNAL_EXPANDO(x) case SIG ## x: return #x; break;
#include "expandos.h"
	}
	return "??";
}

static inline void hook (int s, void (*handler)(int)) {
	struct sigaction act = { 0 };
	sigemptyset(&act.sa_mask);
	act.sa_handler = handler;
	if (0 > sigaction(s, &act, NULL)) {
		printf("Failed to hook %s ", signame(s));
		perror("signal");
		exit(1);
	}
}

static void report (int s) {
	printf("\nsignal SIG%s\n", signame(s));
	which_signal = s;
}

void report_signals (void) {
#define SIGNAL_EXPANDO(x) hook(SIG ## x, &report);
#include "expandos.h"
}
