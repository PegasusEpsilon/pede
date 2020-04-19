/* size_modifiers.c - Pegasus Epsilon's Desktop Environment
** Goal: Less than a megabyte resident
** (C)opyright 2019-2020 "Pegasus Epsilon" <pegasus@pimpninjas.org>
** Distribute Unmodified - https://pegasus.pimpninjas.org/license
*/

#include <X11/Xlib.h>
#include <stdlib.h> 	// abs()
#include <stdint.h> 	// uint32_t
#include <stdio.h>  	// puts()

#include "config.h"
#include "defines.h"
#include "types.h"
#include "util.h"
#include "atoms.h"
#include "wm_core.h"

void keep_on_screen (Window sizing, char side, BOX *t) {
	// constrain windows to viewable area
	if (SIDE_TOP(side) && t->y < 0) { t->h += t->y; t->y = 0; }
	else if (SIDE_BOTTOM(side) && t->y + t->h > root.height)
		t->h = root.height - t->y;
	if (SIDE_LEFT(side) && t->x < 0) { t->w += t->x; t->x = 0; }
	else if (SIDE_RIGHT(side) && t->x + t->w > root.width)
		t->w = root.width - t->x;
}

void snap_to_edges (Window sizing, char side, BOX *t) {
	// snap while resizing (window size varies)
	unsigned snap_w = root.width - t->x;
	unsigned snap_h = root.height - t->y;
	if (SIDE_TOP(side) && abs(t->y) < SNAP) { t->h += t->y; t->y = 0; }
	else if (SIDE_BOTTOM(side) && abs(snap_h - t->h) < SNAP) t->h = snap_h;
	if (SIDE_LEFT(side) && abs(t->x) < SNAP) { t->w += t->x; t->x = 0; }
	else if (SIDE_RIGHT(side) && abs(snap_w - t->w) < SNAP) t->w = snap_w;
}

void snap_to_center (Window sizing, char side, BOX *t) {
	POINT center = (POINT){ .x = root.width / 2, .y = root.height / 2 };;
	POINT target = (POINT){ .x = t->x + t->w, .y = t->y + t->h };

	if (SIDE_TOP(side)) {
		if (abs(center.y - t->y) < SNAP) {
			t->h += t->y - center.y;
			t->y = center.y;
		}
	} else if (SIDE_BOTTOM(side))
		if (abs(center.y - target.y) < SNAP)
			t->h = center.y - t->y;
	if (SIDE_LEFT(side)) {
		if (abs(center.x - t->x) < SNAP) {
			t->w += t->x - center.x;
			t->x = center.x;
		}
	} else if (SIDE_RIGHT(side))
		if (abs(center.x - target.x) < SNAP)
			t->w = center.x - t->x;
}
void snap_to_siblings (Window sizing, char side, BOX *t) {
	Window *windows = NULL;
	unsigned count;

	XQueryTree(display, root.handle, VOID, VOID, &windows, &count);
	if (!windows) return;

	// process sibling windows into pairs of points
	typedef struct { unsigned x, y; } POINT;
	POINT *siblings = NULL;
	unsigned sibling_count = 0;
	for (unsigned i = 0; i < count; i++) {
		if (sizing == windows[i]) continue;
		XWindowAttributes attrs;
		XGetWindowAttributes(display, windows[i], &attrs);
		if (IsViewable != attrs.map_state) continue;
		siblings = realloc(siblings, (2 + sibling_count) * sizeof(*siblings));
		XGetGeometry(display, windows[i], VOID,
			(signed *)&siblings[sibling_count].x,
			(signed *)&siblings[sibling_count].y,
			&siblings[sibling_count + 1].x, &siblings[sibling_count + 1].y,
			VOID, VOID);
		siblings[sibling_count + 1].x += siblings[sibling_count].x;
		siblings[sibling_count + 1].y += siblings[sibling_count].y;
		printf("window 0x%08lx occupies space from %d to %d, %d to %d\n",
			windows[i],
			siblings[sibling_count].x,
			siblings[sibling_count].y,
			siblings[sibling_count + 1].x,
			siblings[sibling_count + 1].y
		);
		sibling_count += 2;
	}
	XFree(windows);
	if (!sibling_count) return;

	POINT target = (POINT){ .x = t->x + t->w, .y = t->y + t->h };

	struct { unsigned closest, delta; } snap = { .delta = -1 };
	unsigned delta;

	if (SIDE_TOP(side)) {
		for (unsigned i = 0; i < sibling_count; i++) {
			delta = abs(t->y - siblings[i].y);
			if (snap.delta > delta) {
				snap.delta = delta;
				snap.closest = siblings[i].y;
			}
		}
		printf("top delta: %d\n", snap.delta);
		if (snap.delta < SNAP) {
			puts("snapping top");
			t->h += t->y - snap.closest;
			t->y = snap.closest;
		}
	} else if (SIDE_BOTTOM(side)) {
		for (unsigned i = 0; i < sibling_count; i++) {
			delta = abs(target.y - siblings[i].y);
			if (snap.delta > delta) {
				snap.delta = delta;
				snap.closest = siblings[i].y;
			}
		}
		printf("bottom delta: %d\n", snap.delta);
		if (snap.delta != (unsigned)-1 && snap.delta < SNAP) {
			puts("snapping bottom");
			t->h = snap.closest - t->y;
		}
	}
	snap.delta = -1;
	if (SIDE_LEFT(side)) {
		for (unsigned i = 0; i < sibling_count; i++) {
			delta = abs(t->x - siblings[i].x);
			if (snap.delta > delta) {
				snap.delta = delta;
				snap.closest = siblings[i].x;
			}
		}
		printf("left delta: %d\n", snap.delta);
		if (-1 != snap.delta && snap.delta < SNAP) {
			puts("snapping left");
			t->w += t->x - snap.closest;
			t->x = snap.closest;
		}
	} else if (SIDE_RIGHT(side)) {
		for (unsigned i = 0; i < sibling_count; i++) {
			delta = abs(target.x - siblings[i].x);
			if (snap.delta > delta) {
				snap.delta = delta;
				snap.closest = siblings[i].x;
			}
		}
		printf("right delta: %d\n", snap.delta);
		if (-1 != snap.delta && snap.delta < SNAP) {
			puts("snapping right");
			t->w = snap.closest - t->x;
		}
	}

	free(siblings);
}
//void do_nothing (Window w, char s, BOX *b) {}
void (*size_modifiers[])(Window sizing, char side, BOX *) = {
	//do_nothing,
	keep_on_screen,
	snap_to_edges,
	snap_to_siblings,
	snap_to_center
};
unsigned size_modifiers_length = sizeof(size_modifiers) / sizeof(void (*));
