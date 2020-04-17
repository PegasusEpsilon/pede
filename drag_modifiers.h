/* drag_modifiers.h - Pegasus Epsilon's Desktop Environment
** Goal: Less than a megabyte resident
** (C)opyright 2019-2020 "Pegasus Epsilon" <pegasus@pimpninjas.org>
** Distribute Unmodified - https://pegasus.pimpninjas.org/license
*/

#ifndef DRAG_MODIFIERS_H
#define DRAG_MODIFIERS_H

typedef struct { int x, y; unsigned w, h; } BOX;

void (*drag_modifiers[])(BOX *);

#endif // DRAG_MODIFIERS_H
