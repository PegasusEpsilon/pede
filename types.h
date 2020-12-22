/* types.h - Pegasus Epsilon's Desktop Environment
** Goal: Less than a megabyte resident
** (C)opyright 2019-2020 "Pegasus Epsilon" <pegasus@pimpninjas.org>
** Distribute Unmodified - https://pegasus.pimpninjas.org/license
*/

#ifndef TYPES_H
#define TYPES_H

#include <X11/Xlib.h>

typedef Bool Unless (long unsigned);
typedef long Workspace;
typedef struct { int x, y; } POINT;
typedef struct { unsigned w, h; } SIZE;
typedef struct { POINT pos; SIZE size; } BOX;
typedef struct { int x, y; unsigned btn; } CLICK;

#endif // TYPES_H
