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

void *XGetWindowPropertyArray (
	Window window, Atom property, Atom type, long unsigned *count, int *bits
) {
	Atom _type;
	void *data = NULL;

	XGetWindowProperty(display, window, property, 0, 0, False, AnyPropertyType,
		&_type, bits, VOID, count, (void *)&data);
	XFree(data);

	if (type != _type || (!*bits && !*count)) return (void *)(*count = 0);
	if (!*bits) {
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
		printf("XLIB ERROR: Zero-bit property occupies %lu bytes. Assuming 32 bits.\n", *count);
		*bits = 32;
	}
	*count /= (long unsigned)*bits / 8;
	XGetWindowProperty(display, window, property, 0, (long)*count, False, type,
		VOID, VOID, VOID, VOID, (void *)&data);

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

	return data;
}

Bool state_exists_internal (Atom needle, Atom *haystack, long unsigned count) {
	while (haystack && count--)
		if (needle == haystack[count])
			return True;
	return False;
}

Bool XWindowPropertyArrayContains (Window window, Atom haystack, Atom needle) {
	long unsigned count;
	int bits;
	Atom *data = XGetWindowPropertyArray(window, haystack, atom[ATOM], &count,
		&bits);

	if (!bits) {
		XFree(data);
		return False;
	}

	Bool ret = state_exists_internal(needle, data, count);
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

Bool window_visible (Window window) {
	XWindowAttributes attrs;
	XGetWindowAttributes(display, window, &attrs);
	return IsViewable == attrs.map_state;
}

unsigned visible_windows (Window **list) {
	unsigned count, src = 0, dst = 0;

	if (!XQueryTree(display, root.handle, VOID, VOID, list, &count)) return 0;
	while (count > src) {
		while (!window_visible((*list)[src]) && count > ++src);
		if (count <= src) break;
		if (src != dst) (*list)[dst] = (*list)[src];
		src++; dst++;
	}

	return dst;
}

Workspace get_workspace (Window window) {
	Workspace ret, *tmp;
	XGetWindowProperty(display, window, atom[_NET_WM_DESKTOP], 0, 1,
		False, atom[CARDINAL], VOID, VOID, VOID, VOID, (void *)&tmp);
	ret = tmp ? *tmp : -1;
	XFree(tmp);
	return ret;
}

Bool window_managed (Window window) {
	return 0 <= get_workspace(window);
/*
	return window_visible(window) || XWindowPropertyArrayContains(
		window, atom[_NET_WM_STATE], atom[_NET_WM_STATE_HIDDEN]);
*/
}

unsigned managed_windows (Window **list) {
	unsigned count, src = 0, dst = 0;
	if (!XQueryTree(display, root.handle, VOID, VOID, list, &count)) return 0;
	while (count > src) {
		while (!window_managed((*list)[src]) && count > ++src);
		if (count <= src) break;
		if (src != dst) (*list)[dst] = (*list)[src];
		src++; dst++;
	}
	return dst;
}

void update_client_list (void) {
	Window *list = NULL;
	int count = (int)managed_windows(&list);
	XChangeProperty(display, root.handle, atom[_NET_CLIENT_LIST], atom[WINDOW],
		32, PropModeReplace, (void *)list, count);
	XFree(list);
}

Window active_window (void) {
	Window *list;
	Window active = None;
	unsigned count;

	if (!XQueryTree(display, root.handle, VOID, VOID, &list, &count)) return 0;
	while (count && list[--count]) {
		// Setting focus on withdrawn/iconified windows is an error.
		// Always consider them inactive.
		if (list[count] != pede && window_visible(list[count])) {
			active = list[count];
			break;
		}
	}
	XFree(list);

	return active;
}

void focus_window (Window window) {
	if (window == pede) return;

	window_diagnostic("Focusing window ", window, "\n");

	XChangeProperty(display, root.handle, atom[_NET_ACTIVE_WINDOW],
		atom[WINDOW], 32, PropModeReplace, (void *)&window, 1);
	XSetInputFocus(display, window, RevertToParent, CurrentTime);
	update_client_list();
}

void focus_active_window (void) {
	XLowerWindow(display, pede);
	Window active = active_window();
	if (active && pede != active && !XWindowPropertyArrayContains(active,
		atom[_NET_WM_STATE], atom[_NET_WM_STATE_ABOVE])) focus_window(active);
}

// add_state_internal called by add_state_raw and toggle_state
// toggle_state by definition checks that the state does not already exist
// therefore checking that the state does not exist here is a waste of time
// therefore add_state_raw must also check that the state does not already
// exist before calling add_state_internal
void add_state_internal (
	Window window, Atom state, Atom *states, long unsigned count
) {
	//if (!states) return;

	size_t size = (++count) * sizeof(state);
	states = realloc(states, size);
	states[count - 1] = state;

	XChangeProperty(display, window, atom[_NET_WM_STATE], atom[ATOM],
		32, PropModeReplace, (void *)states, (int)count);
}

Atom target_state;
Bool state_matcher (Atom state) { return target_state == state; }
void remove_state_internal (
	Window window, Atom state, Atom *states, long unsigned count
) {
	if (!states) return;

	while (count--) {
		if (state == states[count]) {
			target_state = state;
			long unsigned new_count = XDeleteAtomFromArray(states, count, &state_matcher);

			if (new_count) {
				if (new_count < count) {
					window_diagnostic("Window ", window, " ");
					printf("has %d remaining states, updating state property\n", new_count);
					XChangeProperty(display, window, atom[_NET_WM_STATE],
						atom[ATOM], 32, PropModeReplace, (void *)states, (int)new_count);
				} else {
					window_diagnostic("Window ", window, "'s _NET_WM_STATE contains no");
					atom_diagnostic(" ", state, "\n");
				}
			} else {
				window_diagnostic("Window ", window,
					" has zero remaining states, removing state property\n");
				XDeleteProperty(display, window, atom[_NET_WM_STATE]);
			}

			return;
		}
	}

}

// add the state to the given window without any special behavior.
void add_state_raw (Window window, Atom state) {
	long unsigned count;

	// format argument always seems to return zero, even though the docs
	// insist it won't. whatever, just ignore it.
	Atom *states = (Atom *)XGetWindowPropertyArray(window, atom[_NET_WM_STATE],
		atom[ATOM], &count, VOID);

	if (!state_exists_internal(state, states, count))
		add_state_internal(window, state, states, count);

	XFree(states);
}

void remove_state_raw (Window window, Atom state) {
	int bits;
	long unsigned count;

	Atom *states = XGetWindowPropertyArray(window, atom[_NET_WM_STATE],
		atom[ATOM], &count, &bits);

	if (bits) remove_state_internal(window, state, states, count);

	XFree(states);
}

void hide_window (Window win) {
	add_state_raw(win, atom[_NET_WM_STATE_HIDDEN]);
	XUnmapWindow(display, win);
}

void show_window (Window win) {
	remove_state_raw(win, atom[_NET_WM_STATE_HIDDEN]);
	XMapWindow(display, win);
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
	add_state_raw(window, atom[_NET_WM_STATE_FULLSCREEN]);
}

void restore_window (Window window) {
	XSizeHints sizehints;
	XGetWMSizeHints(display, window, &sizehints, VOID, atom[WM_NORMAL_HINTS]);
	if ((USPosition | USSize) & sizehints.flags)
		XMoveResizeWindow(display, window, sizehints.x, sizehints.y,
			(unsigned)sizehints.width, (unsigned)sizehints.height);
	remove_state_raw(window, atom[_NET_WM_STATE_FULLSCREEN]);
}

// public method -- add state to given window WITH special behavior
void add_state (Window window, Atom state) {
	// special states that mean things to us
	if (state == atom[_NET_WM_STATE_FULLSCREEN]) {
		maximize_window(window);
		return;
	}
	if (state == atom[_NET_WM_STATE_HIDDEN]) {
		hide_window(window);
		return;
	}
	// otherwise just do the thing
	add_state_raw(window, state);
}

void remove_state (Window window, Atom state) {
	// special states that mean things to us
	if (state == atom[_NET_WM_STATE_FULLSCREEN]) {
		restore_window(window);
		return;
	}
	if (state == atom[_NET_WM_STATE_HIDDEN]) {
		hide_window(window);
		return;
	}
	// otherwise do the thing
	remove_state_raw (window, state);
}

void toggle_state (Window window, Atom state) {
	long unsigned count;
	int bits;

	atom_diagnostic("Toggle state ", state, " ");
	window_diagnostic("on window ", window, "\n");

	Atom *states = XGetWindowPropertyArray(window, atom[_NET_WM_STATE],
		atom[ATOM], &count, &bits);

	if (bits) {
		if (state_exists_internal(state, states, count))
			remove_state_internal(window, state, states, count);
		else add_state_internal(window, state, states, count);
	}

	XFree(states);
	return;
}

void toggle_fullscreen (void) {
	toggle_state(active_window(), atom[_NET_WM_STATE_FULLSCREEN]);
}

void set_sticky (Window window) {
	add_state(window, atom[_NET_WM_STATE_STICKY]);
	XDeleteProperty(display, window, atom[_NET_WM_DESKTOP]);
}

void activate_workspace (const Workspace which) {
	if ((uint32_t)-1 != which)
		XChangeProperty(display, root.handle, atom[_NET_CURRENT_DESKTOP],
			atom[CARDINAL], 32, PropModeReplace, (void *)&which, 1);

	Window *windows;
	long unsigned count = managed_windows(&windows);

	/* all windows visible */
	if ((uint32_t)-1 == which)
		for (long unsigned i = 0; i < count; i++)
			show_window(windows[i]);
	else for (long unsigned i = 0; i < count; i++) {
		// show all sticky windows
		if (XWindowPropertyArrayContains(windows[i], atom[_NET_WM_STATE],
			atom[_NET_WM_STATE_STICKY])) {
			show_window(windows[i]);
			continue;
		}

		// show all windows visible on the newly activated workspace
		Workspace *workspace = NULL;
		XGetWindowProperty(display, windows[i], atom[_NET_WM_DESKTOP], 0, 1,
			False, atom[CARDINAL], VOID, VOID, VOID, VOID, (void *)&workspace);
		if (!workspace) continue; /* window is not managed by pede */
		if (which == *workspace || (uint32_t)-1 == *workspace)
			show_window(windows[i]);
		else hide_window(windows[i]);
		XFree(workspace);
	}

	XFree(windows);
	//focus_active_window();
}

void set_workspace (Window window, Workspace workspace) {
	if (window == pede) return;
	XChangeProperty(display, window, atom[_NET_WM_DESKTOP], atom[CARDINAL],
		32, PropModeReplace, (void *)&workspace, 1);
	activate_workspace(active_workspace());
}

void close_window (Window window) {
	if (XWindowPropertyArrayContains(
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
			remove_state(event.window, (Atom)event.data.l[1]);
			if (event.data.l[2])
				remove_state(event.window, (Atom)event.data.l[2]);
		break;
		case 1: // add
			add_state(event.window, (Atom)event.data.l[1]);
			if (event.data.l[2])
				add_state(event.window, (Atom)event.data.l[2]);
		break;
		case 2: // toggle
			toggle_state(event.window, (Atom)event.data.l[1]);
			if (event.data.l[2])
				toggle_state(event.window, (Atom)event.data.l[2]);
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
