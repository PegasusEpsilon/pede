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

void keep_on_screen (Window ignored, char side, BOX *t) {
	ignored = ignored; // stfu gcc
	// constrain windows to viewable area
	if (SIDE_TOP(side) && t->pos.y < 0) {
		t->size.h += (unsigned)t->pos.y;
		t->pos.y = 0;
	} else if (SIDE_BOTTOM(side) && (unsigned)t->pos.y + t->size.h > root.height)
		t->size.h = root.height - (unsigned)t->pos.y;
	if (SIDE_LEFT(side) && t->pos.x < 0) {
		t->size.w += (unsigned)t->pos.x;
		t->pos.x = 0;
	} else if (SIDE_RIGHT(side) && (unsigned)t->pos.x + t->size.w > root.width)
		t->size.w = root.width - (unsigned)t->pos.x;
}

void snap_to_edges (Window ignored, char side, BOX *t) {
	ignored = ignored; // stfu gcc
	// snap while resizing (window size varies)
	unsigned snap_w = root.width - (unsigned)t->pos.x;
	unsigned snap_h = root.height - (unsigned)t->pos.y;
	if (SIDE_TOP(side) && abs(t->pos.y) < SNAP) {
		t->size.h += (unsigned)t->pos.y;
		t->pos.y = 0;
	} else if (SIDE_BOTTOM(side) && abs((int)snap_h - (int)t->size.h) < SNAP)
		t->size.h = snap_h;
	if (SIDE_LEFT(side) && abs(t->pos.x) < SNAP) {
		t->size.w += (unsigned)t->pos.x;
		t->pos.x = 0;
	} else if (SIDE_RIGHT(side) && abs((int)snap_w - (int)t->size.w) < SNAP)
		t->size.w = snap_w;
}

void snap_to_center (Window ignored, char side, BOX *t) {
	ignored = ignored; // stfu gcc
	POINT center = (POINT){
		.x = (int)root.width / 2,
		.y = (int)root.height / 2
	};
	POINT target = (POINT){
		.x = t->pos.x + (int)t->size.w,
		.y = t->pos.y + (int)t->size.h
	};

	if (SIDE_TOP(side)) {
		if (abs(center.y - t->pos.y) < SNAP) {
			t->size.h += (unsigned)(t->pos.y - center.y);
			t->pos.y = center.y;
		}
	} else if (SIDE_BOTTOM(side))
		if (abs(center.y - target.y) < SNAP)
			t->size.h = (unsigned)(center.y - t->pos.y);
	if (SIDE_LEFT(side)) {
		if (abs(center.x - t->pos.x) < SNAP) {
			t->size.w += (unsigned)(t->pos.x - center.x);
			t->pos.x = center.x;
		}
	} else if (SIDE_RIGHT(side))
		if (abs(center.x - target.x) < SNAP)
			t->size.w = (unsigned)(center.x - t->pos.x);
}

static void snap_to_siblings (Window sizing, char side, BOX *t) {
	Window *windows = NULL;
	unsigned count = filter_windows(&windows, &window_visible);

	// process sibling windows into pairs of points
	POINT *siblings = NULL;
	unsigned sibling_count = 0;
	for (unsigned i = 0; i < count; i++) {
		if (sizing == windows[i]) continue;
		siblings = realloc(siblings, (2 + sibling_count) * sizeof(*siblings));
		XGetGeometry(display, windows[i], VOID,
			&siblings[sibling_count].x,
			&siblings[sibling_count].y,
			(unsigned *)&siblings[sibling_count + 1].x,
			(unsigned *)&siblings[sibling_count + 1].y,
			VOID, VOID);
		siblings[sibling_count + 1].x += siblings[sibling_count].x;
		siblings[sibling_count + 1].y += siblings[sibling_count].y;
		printf("window 0x%08lx occupies space from %d to %d, %d to %d\n",
			windows[i], siblings[sibling_count].x, siblings[sibling_count].y,
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

	struct { unsigned closest, delta; } snap = { .delta = (unsigned)-1 };
	unsigned delta;

	if (SIDE_TOP(side)) {
		for (unsigned i = 0; i < sibling_count; i++) {
			delta = (unsigned)abs(t->pos.y - siblings[i].y);
			if (snap.delta > delta) {
				snap.delta = delta;
				snap.closest = (unsigned)siblings[i].y;
			}
		}
		printf("top delta: %d\n", snap.delta);
		if (snap.delta < SNAP) {
			puts("snapping top");
			t->size.h += (unsigned)t->pos.y - snap.closest;
			t->pos.y = (int)snap.closest;
		}
	} else if (SIDE_BOTTOM(side)) {
		for (unsigned i = 0; i < sibling_count; i++) {
			delta = (unsigned)abs(target.y - siblings[i].y);
			if (snap.delta > delta) {
				snap.delta = delta;
				snap.closest = (unsigned)siblings[i].y;
			}
		}
		printf("bottom delta: %d\n", snap.delta);
		if (snap.delta != (unsigned)-1 && snap.delta < SNAP) {
			puts("snapping bottom");
			t->size.h = snap.closest - (unsigned)t->pos.y;
		}
	}
	snap.delta = (unsigned)-1;
	if (SIDE_LEFT(side)) {
		for (unsigned i = 0; i < sibling_count; i++) {
			delta = (unsigned)abs(t->pos.x - siblings[i].x);
			if (snap.delta > delta) {
				snap.delta = delta;
				snap.closest = (unsigned)siblings[i].x;
			}
		}
		printf("left delta: %d\n", snap.delta);
		if ((unsigned)-1 != snap.delta && snap.delta < SNAP) {
			puts("snapping left");
			t->size.w += (unsigned)t->pos.x - snap.closest;
			t->pos.x = (int)snap.closest;
		}
	} else if (SIDE_RIGHT(side)) {
		for (unsigned i = 0; i < sibling_count; i++) {
			delta = (unsigned)abs(target.x - siblings[i].x);
			if (snap.delta > delta) {
				snap.delta = delta;
				snap.closest = (unsigned)siblings[i].x;
			}
		}
		printf("right delta: %d\n", snap.delta);
		if ((unsigned)-1 != snap.delta && snap.delta < SNAP) {
			puts("snapping right");
			t->size.w = snap.closest - (unsigned)t->pos.x;
		}
	}

	free(siblings);
}

void (*size_modifiers[])(Window sizing, char side, BOX *) = {
	RESIZE_MODIFIERS
};
unsigned size_modifiers_length = sizeof(size_modifiers) / sizeof(void (*));
