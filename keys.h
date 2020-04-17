/* keys.h - Pegasus Epsilon's Desktop Environment
** Goal: Less than a megabyte resident
** (C)opyright 2019-2020 "Pegasus Epsilon" <pegasus@pimpninjas.org>
** Distribute Unmodified - https://pegasus.pimpninjas.org/license
*/

#ifndef KEYS_H
#define KEYS_H

void handle_key_events (XEvent);
void hook_keys (void);
void unhook_keys (void);

#endif // KEYS_H
