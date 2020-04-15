#ifndef SIGNALS_H
#define SIGNALS_H
#include <signal.h>
typedef enum {
#define MAGIC_EXPANDO(x) x,
#include "signallist.h"
} SIG;
char **sig_names;
int *sig_values;
int which_signal;
void report_signals (void);
#endif
