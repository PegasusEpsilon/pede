/* signal_events.h - Pegasus Epsilon's Desktop Environment
** Goal: Less than a megabyte resident
** (C)opyright 2019-2020 "Pegasus Epsilon" <pegasus@pimpninjas.org>
** Distribute Unmodified - https://pegasus.pimpninjas.org/license
*/

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

#endif // SIGNAL_EVENTS_H
