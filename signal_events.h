/* signal_events.h - Pegasus Epsilon's Desktop Environment
** Goal: Less than a megabyte resident
** (C)opyright 2019-2020 "Pegasus Epsilon" <pegasus@pimpninjas.org>
** Distribute Unmodified - https://pegasus.pimpninjas.org/license
*/

#ifndef SIGNAL_EVENTS_H
#define SIGNAL_EVENTS_H

const char *signame (int);
extern int which_signal;
void report_signals (void);

#endif // SIGNAL_EVENTS_H
