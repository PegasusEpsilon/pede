/* move_modifiers.c - Pegasus Epsilon's Desktop Environment
** Goal: Less than a megabyte resident
** (C)opyright 2019-2020 "Pegasus Epsilon" <pegasus@pimpninjas.org>
** Distribute Unmodified - https://pegasus.pimpninjas.org/license
*/

#include <stdlib.h>
#include <stdio.h>

#include "wm_core.h"
#include "atoms.h"
#include "config.h"
#include "util.h"
#include "types.h"
#include "defines.h"

static void keep_on_screen (Window moving, BOX *t) {
	// constrain windows to viewable area
	t->x = MIN(MAX(t->x, 0), root.width - t->w);
	t->y = MIN(MAX(t->y, 0), root.height - t->h);
}

static void snap_to_center (Window moving, BOX *t) {
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

static void snap_to_edges (Window moving, BOX *t) {
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

static void snap_to_siblings (Window moving, BOX *t) {
	Window *windows = NULL;
	unsigned window_count;

	XQueryTree(display, root.handle, VOID, VOID, &windows, &window_count);
	if (!windows) return;

	// process sibling windows into pairs of points
	POINT *siblings = NULL;
	unsigned sibling_count = 0;
	for (unsigned i = 0; i < window_count; i++) {
		if (moving == windows[i]) continue;
		XWindowAttributes attrs;
		XGetWindowAttributes(display, windows[i], &attrs);
		if (IsViewable != attrs.map_state) continue;
		if (XWindowPropertyArrayContains(windows[i], atom[_NET_WM_WINDOW_TYPE],
			atom[_NET_WM_WINDOW_TYPE_DESKTOP])) continue;
		siblings = realloc(siblings, (2 + sibling_count) * sizeof(*siblings));
		XGetGeometry(display, windows[i], VOID,
			(signed *)&siblings[sibling_count].x,
			(signed *)&siblings[sibling_count].y,
			&siblings[sibling_count + 1].x, &siblings[sibling_count + 1].y,
			VOID, VOID);
		// convert width/height to root coordinate space
		siblings[sibling_count + 1].x += siblings[sibling_count].x;
		siblings[sibling_count + 1].y += siblings[sibling_count].y;
		printf("window %d occupies space from %d, %d to %d, %d\n",
			windows[i],
			siblings[sibling_count].x, siblings[sibling_count].y,
			siblings[sibling_count + 1].x, siblings[sibling_count + 1].y
		);

		sibling_count += 2;
	}
	XFree(windows);
	if (!sibling_count) return;

	POINT target = (POINT){ .x = t->x + t->w, .y = t->y + t->h };

	// this initializer is ugly as fuck...
	struct { unsigned closest, delta; } snaps[4] = {
		{ .delta = -1 }, { .delta = -1 }, { .delta = -1 }, { .delta = -1 }
	};
	unsigned delta;

	for (unsigned i = 0; i < sibling_count; i++) {
		delta = abs(t->x - siblings[i].x);
		if (delta < snaps[SIDE_LEFT_BIT].delta) {
			snaps[SIDE_LEFT_BIT].delta = delta;
			snaps[SIDE_LEFT_BIT].closest = siblings[i].x;
		}
		delta = abs(t->y - siblings[i].y);
		if (delta < snaps[SIDE_TOP_BIT].delta) {
			snaps[SIDE_TOP_BIT].delta = delta;
			snaps[SIDE_TOP_BIT].closest = siblings[i].y;
		}
		delta = abs(target.x - siblings[i].x);
		if (delta < snaps[SIDE_RIGHT_BIT].delta) {
			snaps[SIDE_RIGHT_BIT].delta = delta;
			snaps[SIDE_RIGHT_BIT].closest = siblings[i].x;
		}
		delta = abs(target.y - siblings[i].y);
		if (delta < snaps[SIDE_BOTTOM_BIT].delta) {
			snaps[SIDE_BOTTOM_BIT].delta = delta;
			snaps[SIDE_BOTTOM_BIT].closest = siblings[i].y;
		}
	}
	free(siblings);

	printf("closest left, top, right, bottom: %d(%d), %d(%d), %d(%d), %d(%d)\n",
		snaps[SIDE_LEFT_BIT].closest, snaps[SIDE_LEFT_BIT].delta,
		snaps[SIDE_TOP_BIT].closest, snaps[SIDE_TOP_BIT].delta,
		snaps[SIDE_RIGHT_BIT].closest, snaps[SIDE_RIGHT_BIT].delta,
		snaps[SIDE_BOTTOM_BIT].closest, snaps[SIDE_BOTTOM_BIT].delta
	);

	if (snaps[SIDE_BOTTOM_BIT].delta < snaps[SIDE_TOP_BIT].delta) {
		if (snaps[SIDE_BOTTOM_BIT].delta < SNAP)
			t->y = snaps[SIDE_BOTTOM_BIT].closest - t->h;
	} else if (snaps[SIDE_TOP_BIT].delta < SNAP)
		t->y = snaps[SIDE_TOP_BIT].closest;

	if (snaps[SIDE_RIGHT_BIT].delta < snaps[SIDE_LEFT_BIT].delta) {
		if (snaps[SIDE_RIGHT_BIT].delta < SNAP)
			t->x = snaps[SIDE_RIGHT_BIT].closest - t->w;
	} else if (snaps[SIDE_LEFT_BIT].delta < SNAP)
		t->x = snaps[SIDE_LEFT_BIT].closest;
}

void (*move_modifiers[])(Window, BOX *) = {
	keep_on_screen,
	snap_to_edges,
	snap_to_center,
	snap_to_siblings,
};
unsigned move_modifiers_length = sizeof(move_modifiers) / sizeof(void (*));
