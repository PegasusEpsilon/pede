/* events.h - Pegasus Epsilon's Desktop Environment
** Goal: Less than a megabyte resident
** (C)opyright 2019-2020 "Pegasus Epsilon" <pegasus@pimpninjas.org>
** Distribute Unmodified - https://pegasus.pimpninjas.org/license
*/

#ifndef EVENTS_H
#define EVENTS_H

char *event_names[] = {
#define EVENT_EXPANDO(x) [x] = #x,
#include "expandos.h"
};

#endif // EVENTS_H
