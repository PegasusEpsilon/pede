/* wm_core.c - Pegasus Epsilon's Desktop Environment
** Goal: Less than a megabyte resident
** (C)opyright 2019-2020 "Pegasus Epsilon" <pegasus@pimpninjas.org>
** Distribute Unmodified - https://pegasus.pimpninjas.org/license
*/

#include <X11/Xutil.h>
#include <X11/XKBlib.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "types.h"
#include "atoms.h"
#include "config.h"
#include "pager.h"
#include "util.h"

long long nul;
#define VOID ((void *)&nul)
Display *display;
struct { Window handle; unsigned width, height; } root;
Window pede;

void *XGetWindowPropertyString (Window window, Atom property) {
	void *data = NULL;

	XGetWindowProperty(display, window, property, 0, 9999, False,
		AnyPropertyType, VOID, VOID, VOID, VOID, (void *)&data);
	return data;
}

char *window_title (Window window) {
	const char *const unknown = "<unknown>";
	char *title;
	Atom atoms[3] = { atom[CLASS], atom[WM_NAME], atom[_NET_WM_NAME] };
	for (int i = 3; i--;) {
		title = XGetWindowPropertyString(window, atoms[i]);
		if (title) return title;
	}
	char *tmp = malloc(strlen(unknown) + 1);
	strcpy(tmp, unknown);
	return tmp;
}

void window_diagnostic (
	const char *const before, Window window, const char *const after
) {
	char *title = window_title(window);
	printf("%s0x%08x(%s)%s", before, window, title, after);
	fflush(stdout);
	XFree(title);
}

void atom_diagnostic (
	const char *const before, Atom _atom, const char *const after
) {
	char *atom_name = XGetAtomName(display, _atom);
	printf("%s%s(0x%08x)%s", before,
		atom_name ? atom_name : "<invalid>", _atom, after);
	fflush(stdout);
	XFree(atom_name);
}

long unsigned get_window_property_array (
	Window window, Atom property, Atom type, void **data
) {
	int bits;
	Atom _type;
	long unsigned count;

	XGetWindowProperty(display, window, property, 0, 0, False, AnyPropertyType,
		&_type, &bits, VOID, &count, (void *)data);
	XFree(*data);

	if (type != _type || (!bits && !count))
		return (long unsigned)(*data = NULL);

	if (!bits) {
/*
Let us read together from the book of XGetWindowProperty(3). For The LORD said,
and I quote:

If the specified property exists and either you assign AnyPropertyType to the
req_type argument or the specified type matches the actual property type,
XGetWindowProperty returns the actual property type to actual_type_return
and the actual property format (**never zero**) to actual_format_return.

Emphasis mine.

The source for libX11.so::GetProp.c::XGetWindowProperty seems to confirm that
it will never intentionally do this, and will raise an error whenever the
X server tries to do this, yet *somehow* this code still runs...
*/
//		printf("XLIB ERROR: Zero-bit property occupies %lu bytes. Assuming 32 bits.\n", *count);
		bits = 32;
	}
	count /= (long unsigned)bits / 8;
	XGetWindowProperty(display, window, property, 0, (long)count, False, type,
		VOID, VOID, VOID, VOID, (void *)data);

/*
	if (*count) {
		printf("Enumerating %d extant states on", *count);
		window_diagnostic(" window ", window, ":\n");
	}
	for (long unsigned i = 0; i < *count; i--) {
		printf("state %d:", i);
		atom_diagnostic(" ", ((Atom *)data)[i], " or ");
		window_diagnostic("window ", ((Window *)data)[i], "\n");
	}
*/

	return count;
}

Bool window_property_array_contains (Window window, Atom haystack, Atom needle) {
	Atom *data = NULL;
	long unsigned count = get_window_property_array(window, haystack,
		atom[ATOM], (void **)&data);

	if (!count) {
		XFree(data);
		return False;
	}

	Bool ret = atom_array_contains(needle, data, count);
	XFree(data);
	return ret;
}

Workspace active_workspace (void) {
	Workspace ret, *workspace = NULL;
	XGetWindowProperty(display, root.handle, atom[_NET_CURRENT_DESKTOP], 0,
		1, False, atom[CARDINAL], VOID, VOID, VOID, VOID, (void *)&workspace);
	ret = workspace ? *workspace : 0;
	XFree(workspace);
	return ret;
}

Workspace window_workspace (Window window) {
	Workspace ret, *tmp;
	XGetWindowProperty(display, window, atom[_NET_WM_DESKTOP], 0, 1,
		False, atom[CARDINAL], VOID, VOID, VOID, VOID, (void *)&tmp);
	ret = tmp ? *tmp : -1;
	XFree(tmp);
	return ret;
}

Bool window_visible (Window window) {
	XWindowAttributes attrs;
	XGetWindowAttributes(display, window, &attrs);
	return IsViewable == attrs.map_state;
}

// a window is considered managed if it has
// _NET_WM_DESKTOP or _NET_WM_STATE_STICKY
Bool window_managed (Window window) {
	return 0 <= window_workspace(window) || window_property_array_contains(
		window, atom[_NET_WM_STATE], atom[_NET_WM_STATE_STICKY]
	);
}

// windows without a _NET_WM_WINDOW_TYPE are assumed to be ..._TYPE_NORMAL
Atom window_type (Window window) {
	long unsigned *data = NULL;
	Atom type;
	XGetWindowProperty(display, window, atom[_NET_WM_WINDOW_TYPE], 0, 1,
		False, atom[ATOM], VOID, VOID, VOID, VOID, (void *)&data);
	if (data) {
		type = *data;
		XFree(data);
	} else {
		XGetWindowProperty(display, window, atom[WM_TRANSIENT_FOR], 0, 1,
			False, atom[WINDOW], VOID, VOID, VOID, VOID, (void *)&data);
		type = data ?
			atom[_NET_WM_WINDOW_TYPE_DIALOG] :
			atom[_NET_WM_WINDOW_TYPE_NORMAL];
		XFree(data);
	}
	return type;
}

Bool window_type_normal (Window window) {
	return window_type(window) == atom[_NET_WM_WINDOW_TYPE_NORMAL]
		&& window_visible(window);
}

Bool window_type_desktop (Window window) {
	return window_type(window) == atom[_NET_WM_WINDOW_TYPE_DESKTOP]
		&& window_visible(window);
}

// a window can be moved if
// it's visible
// it's type is NORMAL
Bool window_pageable (Window window) {
	return window_type(window) == atom[_NET_WM_WINDOW_TYPE_NORMAL]
		&& window_visible(window);
}

unsigned filter_windows (Window **list, Bool (*filter)(Window window)) {
	unsigned count;
	if (!XQueryTree(display, root.handle, VOID, VOID, list, &count)) return 0;
	return (unsigned)delete_window_from_array(*list, count, filter);
}

void update_client_list (void) {
	Window *list = NULL;
	int count = (int)filter_windows(&list, &window_managed);
	// _NET_CLIENT_LIST should contain all managed windows
	// in order of mapping time and should be updated as a
	// FIFO in the MapNotify and UnmapNotify events
	//
	// _NET_CLIENT_LIST_STACKING is the one that should be updated here
	XChangeProperty(display, root.handle, atom[_NET_CLIENT_LIST_STACKING],
		atom[WINDOW], 32, PropModeReplace, (void *)list, count);
	XFree(list);
}

Window active_window (void) {
	Window *list;
	unsigned count = filter_windows(&list, &window_pageable);
	Window active = count ? list[--count] : None;
	XFree(list);
	return active;
}

Bool should_focus (Window window) {
	Atom do_not_focus[] = {
		atom[_NET_WM_WINDOW_TYPE_MENU], // menus close when focus changes
		atom[_NET_WM_WINDOW_TYPE_NOTIFICATION],
		atom[_NET_WM_WINDOW_TYPE_COMBO], // xfrun4's _COMBO acts like a _MENU
		0
	};
	Atom type = window_type(window);
	for (int i = 0; do_not_focus[i]; i++)
		if (type == do_not_focus[i]) return False;
	return True;
}

void focus_window (Window window) {
	if (!should_focus(window)) return;

	window_diagnostic("Focusing window ", window, "\n");

	XChangeProperty(display, root.handle, atom[_NET_ACTIVE_WINDOW],
		atom[WINDOW], 32, PropModeReplace, (void *)&window, 1);
	XSetInputFocus(display, window, RevertToParent, CurrentTime);
	update_client_list();
}

void focus_active_window (void) {
	XLowerWindow(display, pede);
	Window active = active_window();
	if (active) focus_window(active);
}

// _NET_WM_STATE management
//
// get_window_states = alloc
// atom_array_contains = check
// *_state_special = special
// *_state_internal = actual
// *_state = alloc, check, special, actual
// *_state_direct = alloc, actual

long unsigned get_window_states (Window window, Atom **states) {
	return get_window_property_array(window, atom[_NET_WM_STATE],
		atom[ATOM], (void *)states);
}

void add_window_state_internal (
	Window window, Atom state, Atom **states, long unsigned count
) {
	size_t size = (++count) * sizeof(state);
	*states = realloc(*states, size);
	*states[count - 1] = state;

	XChangeProperty(display, window, atom[_NET_WM_STATE], atom[ATOM],
		32, PropModeReplace, (void *)*states, (int)count);
}

// add a state without side-effects
void add_window_state_direct (Window window, Atom state) {
	Atom *states = NULL;
	long unsigned count = get_window_states(window, &states);

	add_window_state_internal(window, state, &states, count);

	XFree(states);
}

void maximize_window (Window window) {
	union {
		struct { long flags; XWindowAttributes attributes; } hax;
		XSizeHints sizehints;
	} hax = { .hax.flags = USPosition | USSize };
	XGetWindowAttributes(display, window, &hax.hax.attributes);
	XSetWMSizeHints(display, window, &hax.sizehints, atom[WM_NORMAL_HINTS]);
	XkbStateRec state;
	XkbGetState(display, XkbUseCoreKbd, &state);
	printf("fullscreening, mods state: 0x%02x\n", state.mods);
	if (state.mods & Mod1Mask)
		XMoveResizeWindow(display, window, !hax.hax.attributes.x ? 0 :
			hax.hax.attributes.x + hax.hax.attributes.width == (int)root.width ?
				(int)root.width - ALT_FULLSCREEN_WIDTH : (
					(int)root.width - ALT_FULLSCREEN_WIDTH) / 2,
			((int)root.height - ALT_FULLSCREEN_HEIGHT) / 2,
			ALT_FULLSCREEN_WIDTH, ALT_FULLSCREEN_HEIGHT);
	else
		XMoveResizeWindow(display, window, 0, 0, root.width, root.height);

	add_window_state_direct(window, atom[_NET_WM_STATE_FULLSCREEN]);
}

void add_window_state_special (Window window, Atom state) {
	// special states that mean things to us
	if (state == atom[_NET_WM_STATE_FULLSCREEN])
		maximize_window(window);
}

void add_window_state (Window window, Atom state) {
	Atom *states;
	long unsigned count = get_window_states(window, &states);

	if (!atom_array_contains(state, states, count)) {
		add_window_state_special(window, state);
		add_window_state_internal(window, state, &states, count);
	}

	XFree(states);
}

Atom target_state;
Bool state_matcher (Atom state) { return target_state != state; }
void remove_window_state_internal (
	Window window, Atom state, Atom *states, long unsigned count
) {
	if (!states) return;

	while (count--) {
		if (state == states[count]) {
			target_state = state;
			long unsigned new_count = delete_atom_from_array(
				states, count, &state_matcher);

			if (!new_count) {
				window_diagnostic("Window ", window,
					" has zero remaining states, removing state property\n");
				XDeleteProperty(display, window, atom[_NET_WM_STATE]);
			} else if (new_count < count) {
				window_diagnostic("Window ", window, " has ");
				printf("%d remaining states, updating state property\n", new_count);
				XChangeProperty(display, window, atom[_NET_WM_STATE],
					atom[ATOM], 32, PropModeReplace, (void *)states,
					(int)new_count);
			} else {
				window_diagnostic("Window ", window, "'s _NET_WM_STATE ");
				atom_diagnostic("contains no ", state, "\n");
			}

			return;
		}
	}
}

void remove_window_state_direct (Window window, Atom state) {
	Atom *states;
	long unsigned count = get_window_states(window, &states);

	if (count)
		remove_window_state_internal(window, state, states, count);

	XFree(states);
}

void remove_window_state_special (Window window, Atom state);
void remove_window_state (Window window, Atom state) {
	Atom *states;
	long unsigned count = get_window_states(window, &states);

	if (atom_array_contains(state, states, count)) {
		remove_window_state_special(window, state);
		remove_window_state_internal(window, state, states, count);
	}

	XFree(states);
}

void restore_window (Window window) {
	XSizeHints sizehints;
	XGetWMSizeHints(display, window, &sizehints, VOID, atom[WM_NORMAL_HINTS]);
	if ((USPosition | USSize) & sizehints.flags)
		XMoveResizeWindow(display, window, sizehints.x, sizehints.y,
			(unsigned)sizehints.width, (unsigned)sizehints.height);
	remove_window_state_direct(window, atom[_NET_WM_STATE_FULLSCREEN]);
}

void remove_window_state_special (Window window, Atom state) {
	// special states that mean things to us
	if (state == atom[_NET_WM_STATE_FULLSCREEN]) restore_window(window);
}

void toggle_window_state (Window window, Atom state) {
	atom_diagnostic("Toggle state ", state, " ");
	window_diagnostic("on window ", window, "\n");

	Atom *states = NULL;
	unsigned long count = get_window_states(window, &states);

	if (count && atom_array_contains(state, states, count)) {
		remove_window_state_special(window, state);
		remove_window_state_internal(window, state, states, count);
	} else {
		add_window_state_special(window, state);
		add_window_state_internal(window, state, &states, count);
	}

	XFree(states);
}

void toggle_fullscreen (void) {
	toggle_window_state(active_window(), atom[_NET_WM_STATE_FULLSCREEN]);
}

void set_sticky (Window window) {
	add_window_state_direct(window, atom[_NET_WM_STATE_STICKY]);
	XDeleteProperty(display, window, atom[_NET_WM_DESKTOP]);
}

void activate_workspace (const Workspace which) {
	printf("Activating workspace %d\n", which);
	if ((uint32_t)-1 != which)
		XChangeProperty(display, root.handle, atom[_NET_CURRENT_DESKTOP],
			atom[CARDINAL], 32, PropModeReplace, (void *)&which, 1);

	Window *windows;
	long unsigned count = filter_windows(&windows, &window_managed);
	//puts("managed window list:");
	//for (long unsigned i = count; i--;)
	//	window_diagnostic("", windows[i], "\n");

	/* all windows visible */
	if ((uint32_t)-1 == which)
		for (long unsigned i = 0; i < count; i++)
			XMapWindow(display, windows[i]);
	else for (long unsigned i = 0; i < count; i++) {
		// show all sticky windows
		if (window_property_array_contains(windows[i], atom[_NET_WM_STATE],
			atom[_NET_WM_STATE_STICKY])) {
			XMapWindow(display, windows[i]);
			continue;
		}

		// show all windows visible on the newly activated workspace
		Workspace *workspace = NULL;
		XGetWindowProperty(display, windows[i], atom[_NET_WM_DESKTOP], 0, 1,
			False, atom[CARDINAL], VOID, VOID, VOID, VOID, (void *)&workspace);
		if (!workspace) continue; /* window is not managed by pede */
		if (which == *workspace || (uint32_t)-1 == *workspace)
			XMapWindow(display, windows[i]);
		else XUnmapWindow(display, windows[i]);
		XFree(workspace);
	}

	XFree(windows);
}

void set_workspace (Window window, Workspace workspace) {
	if (window == pede) return;
	XChangeProperty(display, window, atom[_NET_WM_DESKTOP], atom[CARDINAL],
		32, PropModeReplace, (void *)&workspace, 1);
	activate_workspace(active_workspace());
}

void close_window (Window window) {
	if (window_property_array_contains(
		window, atom[WM_PROTOCOLS], atom[WM_DELETE_WINDOW]
	)) XSendEvent(
		display, window, False, NoEventMask, (XEvent *)&(XClientMessageEvent){
			.type = ClientMessage, .display = display,
			.window = window, .message_type = atom[WM_PROTOCOLS],
			.format = 32, .data.l = { (long)atom[WM_DELETE_WINDOW] }
		}
	);
	//else XKillClient(display, window);
	else XDestroyWindow(display, window);
}

void alter_window_state (XClientMessageEvent event) {
	switch (event.data.l[0]) {
		case 0: // remove
			remove_window_state(event.window, (Atom)event.data.l[1]);
			if (event.data.l[2])
				remove_window_state(event.window, (Atom)event.data.l[2]);
		break;
		case 1: // add
			add_window_state(event.window, (Atom)event.data.l[1]);
			if (event.data.l[2])
				add_window_state(event.window, (Atom)event.data.l[2]);
		break;
		case 2: // toggle
			toggle_window_state(event.window, (Atom)event.data.l[1]);
			if (event.data.l[2])
				toggle_window_state(event.window, (Atom)event.data.l[2]);
		break;
	}
}

void refuse_selection_request (XSelectionRequestEvent request) {
	// NB: ICCCM § Responsibilities of the Selection Owner
	XSelectionEvent reply = {
		.type = SelectionNotify,
		.serial = request.serial,
		.send_event = True,
		.display = request.display,
		.requestor = request.requestor,
		.selection = request.selection,
		.target = request.target,
		.property = None, // refused
		.time = request.time
	};
	XSendEvent(request.display, request.requestor, False, NoEventMask,
		(XEvent *)&reply);
}

void map_window (XMapRequestEvent *ev) {
	unsigned width, height;
	long unsigned count;
	Workspace *workspace;

	XGetWindowProperty(ev->display, ev->window, atom[_NET_WM_DESKTOP], 0,
		1, False, atom[CARDINAL], VOID, VOID, &count, VOID, (unsigned char **)&workspace);
	if (count) {
		printf("window 0x%08lx is apparently already on workspace %d\n", ev->window, *workspace);
		XFree(workspace);
		return;
	}
	set_workspace(ev->window, active_workspace());

	int x, y;
	XGetGeometry(ev->display, ev->window, VOID,
		&x, &y, &width, &height, VOID, VOID);

	XSizeHints hints;
	XGetWMSizeHints(
		ev->display, ev->window, &hints, VOID, atom[WM_NORMAL_HINTS]
	);

	if (
		(!(hints.flags & PPosition) && !x && !y) ||
		x + (int)width > (int)root.width ||
		y + (int)height > (int)root.height || x < 0 || y < 0
	) XMoveWindow(
		ev->display, ev->window,
		((int)root.width - (int)width) / 2, ((int)root.height - (int)height) / 2
	);
	XMapWindow(ev->display, ev->window);
}

void become_wm (Window WM) {
	XSetSelectionOwner(display, atom[WM_Sn], WM, CurrentTime);
	XSync(display, True);
	if (pede != XGetSelectionOwner(display, atom[WM_Sn])) {
		puts("Failed to acquire window manager status.");
		exit(1);
	}
}
void make_wm (Window WM) {
	Window old_wm = XGetSelectionOwner(display, atom[WM_Sn]);
	if (old_wm) { // If there is a previous window manager
		// Subscribe to window destruction notifications
		XSelectInput(display, root.handle, SubstructureNotifyMask);
		//XSelectInput(display, old_wm, SubstructureNotifyMask);
		// Take over the job of window manager
		/* from ICCCM:
		** Clients attempting to acquire a selection must set the time value of
		** the SetSelectionOwner request to the timestamp of the event
		** triggering the acquisition attempt, not to CurrentTime. A
		** zero-length append to a property is a way to obtain a timestamp for
		** this purpose; the timestamp is in the corresponding PropertyNotify
		** event.
		** ...but this is the originating event, so... ?
		*/
		puts("Taking over window manager duties...");
		fflush(stdout);
		become_wm(WM);
		puts("Waiting for old window manager to exit...");
		fflush(stdout);
		// Wait for old WM window to be destroyed
		XEvent event;
		do {
			XNextEvent(display, &event);
			printf("event: 0x%08lx\n", event.type);
		} while (event.type != DestroyNotify);
		puts("Look at me. I'm the window manager now.");
	} else become_wm(WM);
}
