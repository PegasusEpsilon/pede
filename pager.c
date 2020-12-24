/* pager.c - Pegasus Epsilon's Desktop Environment
** Goal: Less than a megabyte resident
** (C)opyright 2019-2020 "Pegasus Epsilon" <pegasus@pimpninjas.org>
** Distribute Unmodified - https://pegasus.pimpninjas.org/license
*/

#include <X11/Xutil.h>
#include "wm_core.h"
#include "util.h"

Window *list;
unsigned count, current;

void page_windows_start (void) {
	count = filter_windows(&list, &window_pageable);
	current = 0;
	reverse_window_array(list, count);
}

void page_windows (int down) {
	if (!list) page_windows_start();
	if (!down) rotate_window_array_up(list, count);
	else rotate_window_array_down(list, count);
	XRestackWindows(display, list, (int)count);
}

void page_windows_end (void) {
	focus_window(list[0]);
	XFree(list);
	list = NULL;
	count = current = 0;
}
