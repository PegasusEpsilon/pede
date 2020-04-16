/* wm.c - Pegasus Epsilon's Desktop Environment
** Goal: Less than a megabyte resident
** (C)opyright 2019-2020 "Pegasus Epsilon" <pegasus@pimpninjas.org>
** Distribute Unmodified - https://pegasus.pimpninjas.org/license
*/

#include <X11/Xlib.h>
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

unsigned char active_workspace (Display *display) {
	Atom ret_type;
	int ret_format;
	unsigned long ret_nitems, ret_bytes_after;
	uint32_t *ret_workspace = NULL;
	XGetWindowProperty(display, root.handle, atom[_NET_CURRENT_DESKTOP], 0,
		BUFFER_LENGTH, False, atom[CARDINAL], &ret_type, &ret_format,
		&ret_nitems, &ret_bytes_after, (void *)&ret_workspace);
	return ret_workspace ? *ret_workspace: 0;
}

Window active_window (Display *display) {
	Window ret_root, ret_parent, *list;
	unsigned count = 0;
	XWindowAttributes attrs;

	XQueryTree(display, root.handle, &ret_root, &ret_parent, &list, &count);
	for (int i = count; i && list[--i];) {
		XGetWindowAttributes(display, list[i], &attrs);
		// Setting focus on withdrawn/iconified windows is an error.
		// Always consider them inactive.
		if (IsViewable == attrs.map_state) return list[i];
	}
	return None;
}

void focus_window (Display *display, Window window) {
	XSetInputFocus(display, window, RevertToParent, CurrentTime);
	XChangeProperty(display, root.handle, atom[_NET_ACTIVE_WINDOW],
		atom[WINDOW], 32, PropModeReplace, (void *)&window, 1);
}

void focus_active_window (Display *display) {
	Window active = active_window(display);
	if (0 == active) return;
	focus_window(display, active);
}

void activate_workspace (Display *display, const uint32_t which) {
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

	focus_active_window(display);
}

void set_workspace (Display *display, Window window, uint32_t workspace) {
	XChangeProperty(display, window, atom[_NET_WM_DESKTOP], atom[CARDINAL],
		32, PropModeReplace, (void *)&workspace, 1);
	activate_workspace(display, active_workspace(display));
}

unsigned long XDeleteAtomFromArray (
	Atom *array, unsigned long length, Atom element
) {
	unsigned long dst = 0;
	for (unsigned long src = 0; src < length; src++, dst++) {
		while (src < length && array[src] == element) src++;
		if (src != dst) array[dst] = array[src];
	}
	return dst;
}

void *XGetWindowPropertyArray (
	Display *display, Window window, Atom property,
	Atom type, unsigned long *count
) {

	Atom ret_type;
	void *data;
	int bits;
	unsigned long size;

	XGetWindowProperty(display, window, property, 0, 0, False, type, &ret_type,
		&bits, VOID, &size, (void *)&data);
	XFree(data);
	if (type != ret_type) return (void *)(*count = 0);
	*count = size * 8 / bits;
	XGetWindowProperty(display, window, property, 0, *count, False, type,
		VOID, VOID, count, VOID, (void *)&data);

	return data;
}

Bool XWindowPropertyArrayContains (
	Display *display, Window window, Atom haystack, Atom needle
) {
	unsigned long i;
	Atom *data = XGetWindowPropertyArray(display, window, haystack,
		atom[ATOM], &i);
	while (i--)
		if (needle == data[i]) {
			free(data);
			return True;
		}
	return False;
}

void close_window (Display *display, Window window) {
	if (XWindowPropertyArrayContains(
		display, window, atom[WM_PROTOCOLS], atom[WM_DELETE_WINDOW]
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

void remove_state (Display *display, Window window, Atom state) {
	Atom *data;
	unsigned long i;

	char *state_name = XGetAtomName(display, state);
	//printf("Remove state %s from window %d\n", state_name, window);

	data = XGetWindowPropertyArray(display, window,
		atom[_NET_WM_STATE], atom[ATOM], &i);
	if (!i) {
		printf("Can't remove %s from window %d because it has no"
			" _NET_WM_STATEs.\n", state_name, window);
		return;
	}
	XFree(state_name);

	unsigned long new_i = XDeleteAtomFromArray(data, i, state);
	if (new_i != i) {
		if (new_i) XChangeProperty(display, window, atom[_NET_WM_STATE],
			atom[ATOM], 32, PropModeReplace, (void *)&data, new_i);
		else XDeleteProperty(display, window, atom[_NET_WM_STATE]);
	}

	XFree(data);
}

void add_state (Display *display, Window window, Atom state) {
	Atom states[32] = { 0 };
	unsigned long count;

	printf("Add state %s to window %d\n", XGetAtomName(display, state), window);

	// FIXME: state _NET_WM_STATE_FULLSCREEN = "please maximize me"

	XGetWindowProperty(display, window, atom[_NET_WM_STATE], 0, 0, False,
		atom[ATOM], VOID, VOID, &count, VOID, (void *)&states);

	printf("Enumerating %d existing states:\n", count);

	for (unsigned long i = count; i && states[--i]; ) {
		printf("%s\n", XGetAtomName(display, states[i]));
	}
	states[count] = state;
	XChangeProperty(display, window, atom[_NET_WM_STATE], atom[ATOM], 32,
		PropModeReplace, (void *)&states, count + 1);
}

void alter_window_state (XClientMessageEvent event) {
	switch (event.data.l[0]) {
		case 0:
			remove_state(event.display, event.window, event.data.l[1]);
			if (event.data.l[2])
				remove_state(event.display, event.window, event.data.l[2]);
		break;
		case 1:
			add_state(event.display, event.window, event.data.l[1]);
			if (event.data.l[2])
				add_state(event.display, event.window, event.data.l[2]);
		break;
		case 2:
			if (XWindowPropertyArrayContains(event.display, event.window,
				atom[_NET_WM_STATE], event.data.l[1]))
				remove_state(event.display, event.window, event.data.l[1]);
			else add_state(event.display, event.window, event.data.l[1]);
			if (event.data.l[2]) {
				if (XWindowPropertyArrayContains(event.display,
					event.window, atom[_NET_WM_STATE], event.data.l[2]))
					remove_state(event.display, event.window, event.data.l[2]);
				else add_state(event.display, event.window, event.data.l[2]);
			}
		break;
	}
}

void maximize_window (Display *display, Window window) {
	// FIXME: _NET_WM_STATE_FULLSCREEN
	// still trying to figure out wtf to do here...
	int x, y;
	unsigned int w, h;
	XGetGeometry(display, window, NULL, &x, &y, &w, &h, NULL, NULL);
	XChangeProperty(display, window,
		XInternAtom(display, "WM_RESTORE_TOP", False), atom[CARDINAL], 32,
		PropModeReplace, (void *)&y, 1);
	XChangeProperty(display, window,
		XInternAtom(display, "WM_RESTORE_TOP", False), atom[CARDINAL], 32,
		PropModeReplace, (void *)&y, 1);
	XChangeProperty(display, window,
		XInternAtom(display, "WM_RESTORE_LEFT", False), atom[CARDINAL], 32,
		PropModeReplace, (void *)&x, 1);
	XChangeProperty(display, window,
		XInternAtom(display, "WM_RESTORE_HEIGHT", False), atom[CARDINAL], 32,
		PropModeReplace, (void *)&h, 1);
	XChangeProperty(display, window,
		XInternAtom(display, "WM_RESTORE_WIDTH", False), atom[CARDINAL], 32,
		PropModeReplace, (void *)&w, 1);
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
	set_workspace(ev->display, ev->window, active_workspace(ev->display));

	int x, y;
	XGetGeometry(ev->display, ev->window, VOID,
		&x, &y, &width, &height, VOID, VOID);

	if (0 == x && 0 == y) XConfigureWindow(ev->display, ev->window, CWX | CWY,
		&(XWindowChanges){
			.x = (root.width - width) / 2,
			.y = (root.height - height) / 2
		}
	);
	XMapWindow(ev->display, ev->window);
}

void become_wm_unsafe (Display *display, Window WM) {
	XSetSelectionOwner(display, atom[WM_Sn], WM, CurrentTime);
	XSync(display, True);
	if (pede != XGetSelectionOwner(display, atom[WM_Sn])) {
		puts("Failed to acquire window manager status.");
		exit(1);
	}
}
void become_wm (Display *display, Window WM) {
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
		become_wm_unsafe(display, WM);
		puts("Waiting for old window manager to exit...");
		fflush(stdout);
		// Wait for old WM window to be destroyed
		XEvent event;
		do {
			XNextEvent(display, &event);
			printf("event: 0x%08lx\n", event.type);
		} while (event.type != DestroyNotify);
		puts("Look at me. I'm the window manager now.");
	} else become_wm_unsafe(display, WM);
}
