/* signal_events.c - Pegasus Epsilon's Desktop Environment
** Goal: Less than a megabyte resident
** (C)opyright 2019-2020 "Pegasus Epsilon" <pegasus@pimpninjas.org>
** Distribute Unmodified - https://pegasus.pimpninjas.org/license
*/

#define _POSIX_C_SOURCE 200112L	// struct sigaction

#include <signal.h>	// struct sigaction, sigemptyset(), sigaction(), SIG(USR1, USR2, INT)
#include <stdlib.h>	// exit()
#include <stdio.h>	// puts(), printf(), perror(), fflush(), stdout

typedef enum {
#define SIGNAL_EXPANDO(x) x,
#include "expandos.h"
} SIG;
char **sig_names = (char *[]){
#define SIGNAL_EXPANDO(x) [x] = #x,
#include "expandos.h"
};
int *sig_values = (int []){
#define SIGNAL_EXPANDO(x) [x] = SIG ## x,
#include "expandos.h"
};
int which_signal = 0;

static inline void hook (SIG sig, void (*handler)(int)) {
	struct sigaction act = { 0 };
	sigemptyset(&act.sa_mask);
	act.sa_handler = handler;
	if (0 > sigaction(sig_values[sig], &act, NULL)) {
		printf("Failed to hook %s ", sig_names[sig]);
		perror("signal");
		exit(1);
	}
}

void report (int s) { which_signal = s; }

void report_signals (void) {
#define SIGNAL_EXPANDO(x) hook(x, &report);
#include "expandos.h"
}
