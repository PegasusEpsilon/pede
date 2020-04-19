/* move_modifiers.h - Pegasus Epsilon's Desktop Environment
** Goal: Less than a megabyte resident
** (C)opyright 2019-2020 "Pegasus Epsilon" <pegasus@pimpninjas.org>
** Distribute Unmodified - https://pegasus.pimpninjas.org/license
*/

#ifndef MOVE_MODIFIERS_H
#define MOVE_MODIFIERS_H

#include "types.h"

extern void (*move_modifiers[])(Window w, BOX *);
extern unsigned move_modifiers_length;

#endif // MOVE_MODIFIERS_H
