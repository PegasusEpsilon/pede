/* move_modifiers.c - Pegasus Epsilon's Desktop Environment
** Goal: Less than a megabyte resident
** (C)opyright 2019-2020 "Pegasus Epsilon" <pegasus@pimpninjas.org>
** Distribute Unmodified - https://pegasus.pimpninjas.org/license
*/

#include <stdlib.h>
#include <stdio.h>

#include "wm_core.h"
#include "config.h"
#include "util.h"
#include "types.h"

static void keep_on_screen (BOX *t) {
	// constrain windows to viewable area
	t->x = MIN(MAX(t->x, 0), root.width - t->w);
	t->y = MIN(MAX(t->y, 0), root.height - t->h);
}
static void snap_to_center (BOX *t) {
	//POINT s_center = (POINT){ root.width / 2, root.height / 2 };
	//POINT w_center = (POINT){ t->width / 2, t->h / 2 };
	POINT center = (POINT){
		.x = (root.width - t->w) / 2,
		.y = (root.height - t->h) / 2
	};
	if (abs(center.x - t->x) < SNAP)
		t->x = center.x;
	if (abs(center.y - t->y) < SNAP)
		t->y = center.y;
}
static void snap_to_edges (BOX *t) {
	// snap while dragging (window size constant)
	int snap_x = root.width - t->w;
	int snap_y = root.height - t->h;
	int top_snap = abs(t->y);
	int left_snap = abs(t->x);
	int right_snap = abs(snap_x - t->x);
	int bottom_snap = abs(snap_y - t->y);

	if (top_snap <= bottom_snap) {
		if (top_snap < SNAP) t->y = 0;
	} else
		if (bottom_snap < SNAP) t->y = snap_y;

	if (left_snap <= right_snap) {
		if (left_snap < SNAP) t->x = 0;
	} else
		if (right_snap < SNAP) t->x = snap_x;
}
void (*move_modifiers[])(BOX *) = {
	keep_on_screen,
	snap_to_edges,
	snap_to_center,
};
unsigned move_modifiers_length = sizeof(move_modifiers) / sizeof(void (*));