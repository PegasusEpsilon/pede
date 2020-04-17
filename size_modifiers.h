/* size_modifiers.h - Pegasus Epsilon's Desktop Environment
** Goal: Less than a megabyte resident
** (C)opyright 2019-2020 "Pegasus Epsilon" <pegasus@pimpninjas.org>
** Distribute Unmodified - https://pegasus.pimpninjas.org/license
*/

#ifndef SIZE_MODIFIERS_H
#define SIZE_MODIFIERS_H

#include "types.h"

extern void (*size_modifiers[])(BOX *);
extern unsigned size_modifiers_length;

#endif // SIZE_MODIFIERS_H
