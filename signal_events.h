#ifndef SIGNAL_EVENTS_H
#define SIGNAL_EVENTS_H
#include <signal.h>
typedef enum {
#define SIGNAL_EXPANDO(x) x,
#include "expandos.h"
} SIG;
char **sig_names;
int *sig_values;
int which_signal;
void report_signals (void);
#endif
