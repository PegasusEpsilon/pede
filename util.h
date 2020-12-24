/* util.h - Pegasus Epsilon's Desktop Environment
** Goal: Less than a megabyte resident
** (C)opyright 2019-2020 "Pegasus Epsilon" <pegasus@pimpninjas.org>
** Distribute Unmodified - https://pegasus.pimpninjas.org/license
*/

#ifndef UTIL_H
#define UTIL_H

#include "types.h"

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

void rotate_long_array_up (long unsigned *, size_t);
#define rotate_window_array_up(x, y) rotate_long_array_up((long unsigned *)x, y)
#define rotate_atom_array_up(x, y) rotate_long_array_up((long unsigned *)x, y)
void rotate_long_array_down (long unsigned *, size_t);
#define rotate_window_array_down(x, y) rotate_long_array_down((long unsigned *)x, y)
#define rotate_atom_array_down(x, y) rotate_long_array_down((long unsigned *)x, y)

void reverse_long_array (long unsigned *, size_t);
#define reverse_window_array(x, y) reverse_long_array((long unsigned *)x, y)
#define reverse_atom_array(x, y) reverse_long_array((long unsigned *)x, y)

Bool long_array_contains (long unsigned, long unsigned *, size_t);
#define window_array_contains(x, y, z) \
	long_array_contains((long unsigned)x, (long unsigned *)y, z)
#define atom_array_contains(x, y, z) \
	long_array_contains((long unsigned)x, (long unsigned *)y, z)

size_t delete_long_from_array (long unsigned *, size_t, Unless *);
#define delete_window_from_array(x, y, z) \
	delete_long_from_array((long unsigned *)x, y, (Unless *)z)
#define delete_atom_from_array(x, y, z) \
	delete_long_from_array((long unsigned *)x, y, (Unless *)z)

#endif // UTIL_H
