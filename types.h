/* types.h - Pegasus Epsilon's Desktop Environment
** Goal: Less than a megabyte resident
** (C)opyright 2019-2020 "Pegasus Epsilon" <pegasus@pimpninjas.org>
** Distribute Unmodified - https://pegasus.pimpninjas.org/license
*/

#ifndef TYPES_H
#define TYPES_H

typedef struct { int x, y; unsigned w, h; } BOX;
typedef struct { unsigned x, y; } POINT;
typedef struct { unsigned x, y, btn; } CLICK;

#endif // TYPES_H
