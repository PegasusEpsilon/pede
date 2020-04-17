/* size_modifiers.c - Pegasus Epsilon's Desktop Environment
** Goal: Less than a megabyte resident
** (C)opyright 2019-2020 "Pegasus Epsilon" <pegasus@pimpninjas.org>
** Distribute Unmodified - https://pegasus.pimpninjas.org/license
*/

#include <stdlib.h> 	// abs()
#include "wm_core.h"
#include "config.h"
#include "util.h"
#include "types.h"

void keep_on_screen (BOX *t) {
	// constrain windows to viewable area
	if (t->x < 0) { t->w = t->w + t->x; t->x = 0; }
	else if (t->x + t->w > root.width) t->w = root.width - t->x;
	if (t->y < 0) { t->h = t->h + t->y; t->y = 0; }
	else if (t->y + t->h > root.height) t->h = root.height - t->y;
}
void snap_to_edges (BOX *t) {
	// snap while resizing (window size varies)
	unsigned snap_x = root.width - t->x;
	unsigned snap_y = root.height - t->y;
	if (abs(t->x) < SNAP) { t->w = t->w + t->x; t->x = 0; }
	else if (abs(snap_x - t->w) < SNAP) t->w = snap_x;
	if (abs(t->y) < SNAP) { t->h = t->h + t->y; t->y = 0; }
	else if (abs(snap_y - t->h) < SNAP) t->h = snap_y;
}
void (*size_modifiers[])(BOX *) = {
	keep_on_screen,
	snap_to_edges
};
unsigned size_modifiers_length = sizeof(size_modifiers) / sizeof(void (*));
