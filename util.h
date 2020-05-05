/* util.h - Pegasus Epsilon's Desktop Environment
** Goal: Less than a megabyte resident
** (C)opyright 2019-2020 "Pegasus Epsilon" <pegasus@pimpninjas.org>
** Distribute Unmodified - https://pegasus.pimpninjas.org/license
*/

#ifndef UTIL_H
#define UTIL_H

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

unsigned long delete_int64_t_from_array (int64_t *, unsigned long, int64_t);

#include "data_sizes.h"
#if 8 == ATOM_SIZE
#define XDeleteAtomFromArray(a, l, e) delete_int64_t_from_array((int64_t *)(a), l, (int64_t)(e))
#else
#error 8 != ATOM_SIZE - Send me data_sizes.h in a bug report.
#endif
#if 8 == WINDOW_SIZE
#define XDeleteWindowFromArray(a, l, e) \
	delete_int64_t_from_array((int64_t *)(a), l, (int64_t)(e))
#else
#error 8 != WINDOW_SIZE - Send me data_sizes.h in a bug report.
#endif

#endif // UTIL_H
