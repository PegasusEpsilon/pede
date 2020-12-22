/* util.h - Pegasus Epsilon's Desktop Environment
** Goal: Less than a megabyte resident
** (C)opyright 2019-2020 "Pegasus Epsilon" <pegasus@pimpninjas.org>
** Distribute Unmodified - https://pegasus.pimpninjas.org/license
*/

#ifndef UTIL_H
#define UTIL_H

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

size_t XDeleteLongFromArray (long unsigned *, register size_t, Bool (*)(long unsigned));

#define XDeleteWindowFromArray(x, y, z) \
	XDeleteLongFromArray((long unsigned *)x, y, z);
#define XDeleteAtomFromArray(x, y, z) \
	XDeleteLongFromArray((long unsigned *)x, y, z);

#endif // UTIL_H
