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

static void keep_on_screen (Window unused, BOX *t) {
	(void)unused; // stfu
	// constrain windows to viewable area
	t->pos.x = MIN(MAX(t->pos.x, 0), (int)(root.width - t->size.w));
	t->pos.y = MIN(MAX(t->pos.y, 0), (int)(root.height - t->size.h));
}

static void snap_to_center (Window unused, BOX *t) {
	(void)unused; // stfu
	//POINT s_center = (POINT){ root.width / 2, root.height / 2 };
	//POINT w_center = (POINT){ t->width / 2, t->h / 2 };
	POINT center = (POINT){
		.x = (int)(root.width - t->size.w) / 2,
		.y = (int)(root.height - t->size.h) / 2
	};
	if (abs((int)center.x - t->pos.x) < SNAP)
		t->pos.x = (int)center.x;
	if (abs((int)center.y - t->pos.y) < SNAP)
		t->pos.y = (int)center.y;
}

static void snap_to_edges (Window unused, BOX *t) {
	(void)unused; // stfu
	// snap while dragging (window size constant)
	unsigned snap_x = root.width - t->size.w;
	unsigned snap_y = root.height - t->size.h;
	int top_snap = abs(t->pos.y);
	int left_snap = abs(t->pos.x);
	int right_snap = abs((int)snap_x - t->pos.x);
	int bottom_snap = abs((int)snap_y - t->pos.y);

	if (top_snap <= bottom_snap) {
		if (top_snap < SNAP) t->pos.y = 0;
	} else
		if (bottom_snap < SNAP) t->pos.y = (int)snap_y;

	if (left_snap <= right_snap) {
		if (left_snap < SNAP) t->pos.x = 0;
	} else
		if (right_snap < SNAP) t->pos.x = (int)snap_x;
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
		if (window_property_array_contains(windows[i], atom[_NET_WM_WINDOW_TYPE],
			atom[_NET_WM_WINDOW_TYPE_DESKTOP])) continue;
		siblings = realloc(siblings, (2 + sibling_count) * sizeof(*siblings));
		XGetGeometry(display, windows[i], VOID,
			&siblings[sibling_count].x,
			&siblings[sibling_count].y,
			(unsigned int *)&siblings[sibling_count + 1].x,
			(unsigned int *)&siblings[sibling_count + 1].y,
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

	POINT target = (POINT){
		.x = t->pos.x + (int)t->size.w,
		.y = t->pos.y + (int)t->size.h
	};

	// this initializer is ugly as fuck...
	struct { unsigned closest, delta; } snaps[4] = {
		{ .delta = (unsigned)-1 }, { .delta = (unsigned)-1 },
		{ .delta = (unsigned)-1 }, { .delta = (unsigned)-1 }
	};
	unsigned delta;

	for (unsigned i = 0; i < sibling_count; i++) {
		delta = (unsigned)abs(t->pos.x - siblings[i].x);
		if (delta < snaps[SIDE_LEFT_BIT].delta) {
			snaps[SIDE_LEFT_BIT].delta = delta;
			snaps[SIDE_LEFT_BIT].closest = (unsigned)siblings[i].x;
		}
		delta = (unsigned)abs(t->pos.y - siblings[i].y);
		if (delta < snaps[SIDE_TOP_BIT].delta) {
			snaps[SIDE_TOP_BIT].delta = delta;
			snaps[SIDE_TOP_BIT].closest = (unsigned)siblings[i].y;
		}
		delta = (unsigned)abs(target.x - siblings[i].x);
		if (delta < snaps[SIDE_RIGHT_BIT].delta) {
			snaps[SIDE_RIGHT_BIT].delta = delta;
			snaps[SIDE_RIGHT_BIT].closest = (unsigned)siblings[i].x;
		}
		delta = (unsigned)abs(target.y - siblings[i].y);
		if (delta < snaps[SIDE_BOTTOM_BIT].delta) {
			snaps[SIDE_BOTTOM_BIT].delta = delta;
			snaps[SIDE_BOTTOM_BIT].closest = (unsigned)siblings[i].y;
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
			t->pos.y = (int)(snaps[SIDE_BOTTOM_BIT].closest - t->size.h);
	} else if (snaps[SIDE_TOP_BIT].delta < SNAP)
		t->pos.y = (int)snaps[SIDE_TOP_BIT].closest;

	if (snaps[SIDE_RIGHT_BIT].delta < snaps[SIDE_LEFT_BIT].delta) {
		if (snaps[SIDE_RIGHT_BIT].delta < SNAP)
			t->pos.x = (int)(snaps[SIDE_RIGHT_BIT].closest - t->size.w);
	} else if (snaps[SIDE_LEFT_BIT].delta < SNAP)
		t->pos.x = (int)snaps[SIDE_LEFT_BIT].closest;
}

void (*move_modifiers[])(Window, BOX *) = {
	MOVE_MODIFIERS
};
unsigned move_modifiers_length = sizeof(move_modifiers) / sizeof(void (*));
