/* wm.c - Pegasus Epsilon's Desktop Environment
** Goal: Less than a megabyte resident
** (C)opyright 2019-2020 "Pegasus Epsilon" <pegasus@pimpninjas.org>
** Distribute Unmodified - https://pegasus.pimpninjas.org/license
*/

#include <X11/Xutil.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include "atoms.h"
#include "config.h"

long long nul;
#define VOID ((void *)&nul)
#define BUFFER_LENGTH 512
char buffer[BUFFER_LENGTH];
Display *display;
struct { Window handle; unsigned int width, height; } root;
Window pede;

unsigned char active_workspace (void) {
	uint32_t *workspace = NULL;
	XGetWindowProperty(display, root.handle, atom[_NET_CURRENT_DESKTOP], 0,
		1, False, atom[CARDINAL], VOID, VOID, VOID, VOID, (void *)&workspace);
	return workspace ? *workspace: 0;
}

Window active_window (void) {
	Window *list;
	Window active = None;
	unsigned count = 0;
	XWindowAttributes attrs;

	XQueryTree(display, root.handle, VOID, VOID, &list, &count);
	for (int i = count; i && list[--i];) {
		XGetWindowAttributes(display, list[i], &attrs);
		// Setting focus on withdrawn/iconified windows is an error.
		// Always consider them inactive.
		if (IsViewable == attrs.map_state) {
			active = list[i];
			break;
		}
	}
	XFree(list);
	return active;
}

void focus_window (Window window) {
	XSetInputFocus(display, window, RevertToParent, CurrentTime);
	XChangeProperty(display, root.handle, atom[_NET_ACTIVE_WINDOW],
		atom[WINDOW], 32, PropModeReplace, (void *)&window, 1);
}

void focus_active_window (void) {
	Window active = active_window();
	if (0 == active) return;
	focus_window(active);
}

void activate_workspace (const uint32_t which) {
	Window *windows = NULL;
	unsigned int count;

	if ((uint32_t)-1 != which)
		XChangeProperty(display, root.handle, atom[_NET_CURRENT_DESKTOP],
			atom[CARDINAL], 32, PropModeReplace, (void *)&which, 1);

	if (!XQueryTree(display, root.handle, VOID, VOID, &windows,
		&count)) return;

	uint32_t *workspace = NULL;
	for (int i = 0; i < count; i++) {
		XGetWindowProperty(display, windows[i], atom[_NET_WM_DESKTOP], 0,
			BUFFER_LENGTH, False, atom[CARDINAL], VOID, VOID, VOID, VOID,
			(void *)&workspace);
		if (!workspace) continue;
		if (which == *workspace
			|| (uint32_t)-1 == *workspace
			|| (uint32_t)-1 == which)
			XMapWindow(display, windows[i]);
		else XUnmapWindow(display, windows[i]);
		XFree(workspace);
	}

	XFree(windows);

	focus_active_window();
}

void set_workspace (Window window, uint32_t workspace) {
	XChangeProperty(display, window, atom[_NET_WM_DESKTOP], atom[CARDINAL],
		32, PropModeReplace, (void *)&workspace, 1);
	activate_workspace(active_workspace());
}

unsigned long XDeleteAtomFromArray (
	Atom *array, unsigned long length, Atom element
) {
	unsigned long src = 0, dst = 0;
	while (length && src < length) {
		while (src < length && array[src] == element) src++;
		if (src < length && src != dst) array[dst++] = array[src++];
	}
	return dst;
}

void *XGetWindowPropertyArray (
	Window window, Atom property, Atom type, unsigned long *count, int *bits
) {
	Atom _type;
	void *data = NULL;

	XGetWindowProperty(display, window, property, 0, 0, False, type, &_type,
		bits, VOID, count, (void *)&data);
	XFree(data);

	if (type != _type) return (void *)(*count = 0);
	*count /= *bits / 8;
	XGetWindowProperty(display, window, property, 0, *count, False, type,
		VOID, VOID, VOID, VOID, (void *)&data);

	return data;
}

Bool XWindowPropertyArrayContains (Window window, Atom haystack, Atom needle) {
	unsigned long i;
	int bits;
	Atom *data = XGetWindowPropertyArray(
		window, haystack, atom[ATOM], &i, &bits
	);
	while (i--)
		if (needle == data[i]) {
			XFree(data);
			return True;
		}
	XFree(data);
	return False;
}

void close_window (Window window) {
	if (XWindowPropertyArrayContains(
		window, atom[WM_PROTOCOLS], atom[WM_DELETE_WINDOW]
	)) XSendEvent(
		display, window, False, NoEventMask, (XEvent *)&(XClientMessageEvent){
			.type = ClientMessage, .display = display,
			.window = window, .message_type = atom[WM_PROTOCOLS],
			.format = 32, .data.l = { atom[WM_DELETE_WINDOW] }
		}
	);
	//else XKillClient(display, window);
	else XDestroyWindow(display, window);
}

void restore_window (Window window) {
	XSizeHints sizehints;
	XGetWMSizeHints(display, window, &sizehints, VOID, atom[WM_NORMAL_HINTS]);
	if ((USPosition | USSize) & sizehints.flags)
		XMoveResizeWindow(display, window, sizehints.x, sizehints.y,
			sizehints.width, sizehints.height);
}

void remove_state (Window window, Atom state) {
	if (state == atom[_NET_WM_STATE_FULLSCREEN])
		restore_window(window);

	int bits;
	unsigned long count;

	char *state_name = XGetAtomName(display, state);
	Atom *states = XGetWindowPropertyArray(
		window, atom[_NET_WM_STATE], atom[ATOM], &count, &bits
	);
	if (!count) {
		printf("Can't remove %s from window 0x%08lx because it has no"
			" _NET_WM_STATEs.\n", state_name, window);
		XFree(state_name);
		return;
	} else printf("Remove state %s from window 0x%08lx\n", state_name, window);

	if (count) printf("Enumerating %d extant states\n", count);
	for (unsigned long i = 0; i < count; i++) {
		printf("state: %s\n", XGetAtomName(display, states[i]));
		fflush(stdout);
	}

	unsigned long new_count = XDeleteAtomFromArray(states, count, state);
	if (new_count != count) {
		if (new_count) XChangeProperty(display, window, atom[_NET_WM_STATE],
			atom[ATOM], 8 * sizeof(state), PropModeReplace, (void *)&states, new_count);
		else XDeleteProperty(display, window, atom[_NET_WM_STATE]);
	} else printf("Window 0x%08lx's _NET_WM_STATE contains no %s\n", window, state_name);

	XFree(state_name);
	XFree(states);
}

void maximize_window (Window window) {
	union {
		struct { long flags; XWindowAttributes attributes; } hax;
		XSizeHints sizehints;
	} hax = { .hax.flags = USPosition | USSize };
	XGetWindowAttributes(display, window, &hax.hax.attributes);
	XSetWMSizeHints(display, window, &hax.sizehints, atom[WM_NORMAL_HINTS]);
	XMoveResizeWindow(display, window, 0, 0, root.width, root.height);
}

void add_state (Window window, Atom state) {
	if (state == atom[_NET_WM_STATE_FULLSCREEN])
		maximize_window(window);

	printf("Add state %s to window 0x%08lx\n", XGetAtomName(display, state), window);

	unsigned long count;
	Atom *states = (Atom *)XGetWindowPropertyArray(
		window, atom[_NET_WM_STATE], atom[ATOM], &count, VOID
	);

	if (count) printf("Enumerating %d existing states:\n", count);
	for (unsigned long i = 0; i < count && states[i]; i--)
		printf("%d: %s\n", i, XGetAtomName(display, states[i]));

	size_t size = (++count) * sizeof(state);
	states = realloc(states, size);
	states[count - 1] = state;
	XChangeProperty(display, window, atom[_NET_WM_STATE], atom[ATOM],
		4 * sizeof(state), PropModeReplace, (void *)states, count);
	XFree(states);
}

void alter_window_state (XClientMessageEvent event) {
	switch (event.data.l[0]) {
		case 0: // remove
			remove_state(event.window, event.data.l[1]);
			if (event.data.l[2])
				remove_state(event.window, event.data.l[2]);
		break;
		case 1: // add
			add_state(event.window, event.data.l[1]);
			if (event.data.l[2])
				add_state(event.window, event.data.l[2]);
		break;
		case 2: // toggle
			if (XWindowPropertyArrayContains(
				event.window, atom[_NET_WM_STATE], event.data.l[1])
			) remove_state(event.window, event.data.l[1]);
			else add_state(event.window, event.data.l[1]);
			if (event.data.l[2]) {
				if (XWindowPropertyArrayContains(
					event.window, atom[_NET_WM_STATE], event.data.l[2]
				)) remove_state(event.window, event.data.l[2]);
				else add_state(event.window, event.data.l[2]);
			}
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
	unsigned long count;
	unsigned char *workspace;

	XGetWindowProperty(ev->display, ev->window, atom[_NET_WM_DESKTOP], 0,
		BUFFER_LENGTH, False, atom[CARDINAL], VOID, VOID, &count, VOID, &workspace);
	if (count) {
		printf("window 0x%08lx is apparently already on workspace %d\n", ev->window, *workspace);
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

	if (!(hints.flags & PPosition) && !x && !y) XMoveWindow(
		ev->display, ev->window,
		(root.width - width) / 2, (root.height - height) / 2
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
