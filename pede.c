/* pede.c - Pegasus Epsilon's Desktop Environment
** Goal: Less than a megabyte resident
** (C)opyright 2019-2020 "Pegasus Epsilon" <pegasus@pimpninjas.org>
** Distribute Unmodified - https://pegasus.pimpninjas.org/license
*/

#include <X11/Xutil.h>

#include <errno.h>  	// errno
#include <unistd.h> 	// getpid()

#include <stdio.h>  	// puts(), printf(), fflush(), stdout
#include <stdlib.h> 	// exit()
#include <stdbool.h>
#include <stdint.h> 	// uint32_t

#include <libgen.h> 	// dirname()
#include <string.h> 	// strlen()
#include <sys/select.h>	// fd_set, pipe(), FD_SET(), FD_ZERO(), select()
#include <time.h>   	// nanosleep()

#include "atoms.h"
#include "events.h"
#include "signals.h"

// TODO: break everything out into modules, then write:
// inter-window snapping/gluing, generalized keyboard shortcuts
// better alt-tab, fullscreen support, window restoration after fullscreen,
// vastly improve ICCCM and EWMH support...
// tiling module? windows 10-style edge tiling?

// desktop button
#define FILENAME "power.icon"
#define BPP 32
#define HEIGHT 32
#define WIDTH 32
#define BOTTOM 0
#define LEFT 0

// RUNNERARGS should be comma-separated and individually quoted ie:
//#define RUNNERARGS "arg1", "arg2", "arg3"
#define RUNNER "xfrun4"
#define RUNNERARGS "--disable-server"

// minimum window size (height and width) - do not go below 1
#define MINIMUM_SIZE 100
// snap distance, 0 to disable
#define SNAP 25
#define MAX(a, b) ((a) > (b) ? (a) : (b))

static long long nul;
#define VOID ((void *)&nul)
static Window pede = 0;
static struct { Window handle; unsigned int width, height; } root;

#ifdef DEBUG
static int (*saved_XChangeProperty)(Display *, Window, Atom, Atom, int, int,
	const unsigned char *, int) = 0;
void enable_debug (void) { saved_XChangeProperty = XChangeProperty; }
int debug_XChangeProperty (Display *display, Window w, Atom property,
	Atom type, int format, int mode, unsigned char *data, int nelements) {
	printf("XChangeProperty request #%d: %s(%d)\n", XNextRequest(display), XGetAtomName(display, property), property);
	fflush(stdout);
	int ret = saved_XChangeProperty(display, w, property, type, format, mode,
		data, nelements);
	return ret;
}
#define XChangeProperty debug_XChangeProperty
#endif

#define BUFFER_LENGTH 512
char buffer[BUFFER_LENGTH];
int death_proof (Display *display, XErrorEvent *error) {
	XGetErrorText(display, error->error_code, buffer, BUFFER_LENGTH-1);
	printf("X request %d: Error %d/%d.%d: %s\n", error->serial, error->error_code,
		error->request_code, error->minor_code, buffer);
	fflush(stdout);
	return 0;
}

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

void activate_workspace (Display *display, const uint32_t which) {
	Window *windows = NULL;
	unsigned int count;

	//printf("activate workspace %d\n", which);
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

	//focus_active_window(display);
}

void set_workspace (Display *display, Window window, uint32_t workspace) {
	XChangeProperty(display, window, atom[_NET_WM_DESKTOP], atom[CARDINAL],
		32, PropModeReplace, (void *)&workspace, 1);
	activate_workspace(display, active_workspace(display));
}

Window active_window (Display *display) {
	Window ret_root, ret_parent, *list;
	unsigned count;
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
	printf("focusing window 0x%08lx\n", active);
	focus_window(display, active);
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
		do XNextEvent(display, &event);
		while (event.type != DestroyNotify);
		puts("Look at me. I'm the window manager now.");
	} else become_wm_unsafe(display, WM);
}

void set_window_name (Display *display, Window w, char *name) {
	XTextProperty textprop;
	XStringListToTextProperty(&name, 1, &textprop);
	XStoreName(display, w, name);
	XSetWMName(display, w, &textprop);
	XChangeProperty(display, w, XInternAtom(display, "_NET_WM_NAME", False),
		atom[UTF8_STRING], 8, PropModeReplace,
		(unsigned char *)name, strlen(name));
}

typedef union {
	uint32_t packed;
	struct { uint8_t r, g, b, a; } c;
} __attribute__((packed)) PIXEL;

XImage *load_image (Display *display, int screen, Visual *visual, char *filename) {
	PIXEL *data = malloc(WIDTH * HEIGHT * sizeof(PIXEL));

	FILE *file = fopen(filename, "r");
	free(filename);
	if (!file) {
		perror("failed opening " FILENAME);
		fflush(stdout);
		exit(1);
	}
	fread(data, sizeof(PIXEL), WIDTH * HEIGHT, file);
	fclose(file);

	for (uint32_t i = 0; i < WIDTH * HEIGHT; i++) {
		// fix byte order
		data[i].c.r ^= data[i].c.b;
		data[i].c.b ^= data[i].c.r;
		data[i].c.r ^= data[i].c.b;
		// pre-multiply alpha
		data[i].c.r *= data[i].c.a / 255;
		data[i].c.g *= data[i].c.a / 255;
		data[i].c.b *= data[i].c.a / 255;
	}

	return XCreateImage(display, visual, BPP, ZPixmap, 0,
		(void *)data, WIDTH, HEIGHT, BPP, WIDTH * sizeof(PIXEL));
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
	XGetGeometry(ev->display, ev->window, VOID, VOID, VOID, &width, &height, VOID, VOID);
	XConfigureWindow(ev->display, ev->window, CWX | CWY,
		&(XWindowChanges){
		.x = (root.width - width) / 2,
		.y = (root.height - height) / 2
	});
	XMapWindow(ev->display, ev->window);
}

char *argv0 = NULL;
bool signal_handler (void) {
	printf("received signal %d\n", which_signal);
	switch (which_signal) {
	case SIGINT:
		puts("SIGINT");
		return true;
	case SIGUSR1:
		execlp(argv0, argv0);
		// we never get here anyway
	}
	fflush(stdout);
	return false;
}

void XWaitEvent (Display *display) {
	int Xfd = ConnectionNumber(display);
	fd_set listen, pending;
	FD_ZERO(&listen);
	FD_SET(Xfd, &listen);
	while (!XPending(display)) {
		pending = listen;
		if (0 >= select(FD_SETSIZE, &pending, NULL, NULL, NULL))
			if (signal_handler()) return;
	}
}

typedef enum { KcTab, KcF4, KcR, Kc1, Kc2, Kc3, Kc4, Kc_LAST } WatchedKeyCode;
KeyCode keycodes[Kc_LAST];
char *keycode_names[] = {
	[KcTab] = "Tab", [KcF4] = "F4", [KcR] = "R",
	[Kc1] = "1", [Kc2] = "2", [Kc3] = "3", [Kc4] = "4",
};

void event_loop (Display *display, Window pede, GC gc, XImage *img) {
	XEvent event;
	XWindowAttributes attrStart;
	XButtonEvent dragStart = { 0 };
	char moveSide = 0;
	for (;;) { // ever
		XWaitEvent(display);
		if (!XPending(display)) return;
		XNextEvent(display, &event);
		printf("event %s next request %d\n", event_names[event.type], XNextRequest(display)); fflush(stdout);
		switch (event.type) {
		case MapNotify:
			if (event.xmaprequest.window == pede) {
				puts("I have arrived.");
				XLowerWindow(display, pede);
			}
			break;
		case ConfigureRequest:
			//printf("ConfigureRequest window: 0x%08lx\n", event.xconfigurerequest.window);
			XConfigureWindow(
				event.xconfigurerequest.display,
				event.xconfigurerequest.window,
				event.xconfigurerequest.value_mask,
				&(XWindowChanges){
					.x = event.xconfigurerequest.x,
					.y = event.xconfigurerequest.y,
					.width = event.xconfigurerequest.width,
					.height = event.xconfigurerequest.height,
					.border_width = event.xconfigurerequest.border_width,
					.sibling = event.xconfigurerequest.above,
					.stack_mode = event.xconfigurerequest.detail,
				});
			XSync(display, False);
			break;
		case Expose:
			XPutImage(display, pede, gc, img, 0, 0, 0, 0, WIDTH, HEIGHT);
			XSync(display, False);
			break;
		case SelectionRequest: // ICCCM compliance, request always refused
			refuse_selection_request(event.xselectionrequest);
			break;
		case ClientMessage:
			// switch to given workspace
			if (event.xclient.message_type == atom[_NET_CURRENT_DESKTOP])
				activate_workspace(event.xclient.display,
					*event.xclient.data.l);
			// move window to given workspace
			else if (event.xclient.message_type == atom[_NET_WM_DESKTOP])
				set_workspace(event.xclient.display, event.xclient.window,
					*event.xclient.data.l);
			// add or remove various states to _NET_WM_STATE properties
			else if (event.xclient.message_type == atom[_NET_WM_STATE])
				alter_window_state(event.xclient);
			// idfk
			else printf("Received unmonitored client message: %s/%d\n",
				XGetAtomName(display, event.xclient.message_type),
				*event.xclient.data.l);
			break;
		case MapRequest:
			// FIXME: why do chromium --app=... windows not display?
			map_window(&event.xmaprequest);
			set_workspace(event.xmaprequest.display, event.xmaprequest.window,
				active_workspace(event.xmaprequest.display));
			focus_window(event.xmaprequest.display, event.xmaprequest.window);
			break;
		case DestroyNotify: // fall through
		case CirculateNotify:
			focus_active_window(event.xcirculate.display);
			break;
		case SelectionClear:
			if (event.xselectionclear.selection == atom[WM_Sn]) {
				puts("Lost window manager status, cleaning up...");
				return;
			}
			break;
		case KeyPress:
			// stupid C rules about switch blocks requiring integer constants...
			if (event.xkey.keycode == keycodes[KcTab]) {//do {
				// TODO: change this to manually select and raise
				// the bottom window or lower the top window
				XCirculateSubwindows(event.xkey.display, event.xkey.root,
				event.xkey.state & 1 ? LowerHighest : RaiseLowest);
				XLowerWindow(event.xkey.display, pede);
			}// while (active_window(event.xkey.display) == pede);
			else if (event.xkey.keycode == keycodes[KcR]) {
				if (!fork()) execlp(RUNNER, RUNNER, RUNNERARGS);
			} else if (event.xkey.keycode == keycodes[KcF4])
				close_window(event.xkey.display,
					active_window(event.xkey.display));
			else if (event.xkey.keycode == keycodes[Kc1]) {
				if (event.xkey.state & ShiftMask)
					set_workspace(event.xkey.display, active_window(event.xkey.display), 0);
				else activate_workspace(event.xkey.display, 0);
			} else if (event.xkey.keycode == keycodes[Kc2]) {
				if (event.xkey.state & ShiftMask)
					set_workspace(event.xkey.display, active_window(event.xkey.display), 1);
				else activate_workspace(event.xkey.display, 1);
			} else if (event.xkey.keycode == keycodes[Kc3]) {
				if (event.xkey.state & ShiftMask)
					set_workspace(event.xkey.display, active_window(event.xkey.display), 2);
				else activate_workspace(event.xkey.display, 2);
			} else if (event.xkey.keycode == keycodes[Kc4]) {
				if (event.xkey.state & ShiftMask)
					set_workspace(event.xkey.display, active_window(event.xkey.display), 3);
				else activate_workspace(event.xkey.display, 3);
			}
			break;
		case ButtonPress:
			if (None == event.xbutton.subwindow) {
				XAllowEvents(display, ReplayPointer, event.xbutton.time);
				break;
			}
			if (pede == event.xbutton.subwindow) return;
			XRaiseWindow(display, event.xbutton.subwindow);
			XSetInputFocus(display, event.xbutton.subwindow,
				RevertToParent, event.xbutton.time);
			dragStart = event.xbutton;
			if (0 == dragStart.state
				&& 9 != dragStart.button) {
				XAllowEvents(display, ReplayPointer, event.xbutton.time);
				break;
			}
			XGrabPointer(event.xbutton.display, event.xbutton.subwindow,
					True, PointerMotionMask | ButtonReleaseMask,
					GrabModeAsync, GrabModeAsync, None, None, event.xbutton.time);
			XGetWindowAttributes(display, event.xbutton.subwindow, &attrStart);

			if (1 == dragStart.button || 9 == dragStart.button) break; // move

			// event.xbutton.x and event.xbutton.y are not consistent
			int relative_x = dragStart.x_root - attrStart.x;
			int relative_y = dragStart.y_root - attrStart.y;

			moveSide = // calculate which nonant the click occurred in
				((relative_x > attrStart.width / 3) << 0) | // right
				((relative_y < 2 * attrStart.height / 3) << 1) | // top
				((relative_x < 2 * attrStart.width / 3) << 2) | // left
				((relative_y > attrStart.height / 3) << 3); // bottom

			// if side nonant clicked, move only that side
			switch (moveSide) {
				case  7: moveSide = 2; break;
				case 11: moveSide = 1; break;
				case 13: moveSide = 8; break;
				case 14: moveSide = 4; break;
			}
			break;
		case MotionNotify:
			while (XCheckTypedEvent(display, MotionNotify, &event));
			int xdiff = event.xbutton.x_root - dragStart.x_root;
			int ydiff = event.xbutton.y_root - dragStart.y_root;
			struct { int x, y, w, h; } target;

			if (1 == dragStart.button || 9 == dragStart.button) { // move
				target.x = attrStart.x + xdiff;
				target.y = attrStart.y + ydiff;
				target.w = attrStart.width;
				target.h = attrStart.height;

				// snap while dragging (window size constant)
				int top_snap = abs(target.y);
				int right_snap = abs(root.width - target.x - target.w);
				int bottom_snap = abs(root.height - target.y - target.h);
				int left_snap = abs(target.x);

				if (top_snap <= bottom_snap) {
					if (top_snap < SNAP) target.y = 0;
				} else if (bottom_snap < SNAP)
					target.y = root.height - target.h;

				if (left_snap <= right_snap) {
					if (left_snap < SNAP) target.x = 0;
				} else if (right_snap < SNAP)
						target.x = root.width - target.w;

			} else {
				// if center nonant clicked, always grow first
				if (15 == moveSide) {
					moveSide =
						((xdiff > 0) << 0) |
						((ydiff < 0) << 1) |
						((xdiff < 0) << 2) |
						((ydiff > 0) << 3);
					int xmag = abs(xdiff);
					int ymag = abs(ydiff);
					if (xmag / 2 > ymag) moveSide &= 5;
					if (ymag / 2 > xmag) moveSide &= 10;
				}

				target.x = attrStart.x + ((1 << 2) & moveSide ? xdiff : 0);
				target.y = attrStart.y + ((1 << 1) & moveSide ? ydiff : 0);
				target.w = MAX(attrStart.width
					+ ((1 << 0) & moveSide ? xdiff : 0)
					- ((1 << 2) & moveSide ? xdiff : 0), MINIMUM_SIZE);
				target.h = MAX(attrStart.height
					+ ((1 << 3) & moveSide ? ydiff : 0)
					- ((1 << 1) & moveSide ? ydiff : 0), MINIMUM_SIZE);

				// snap while resizing (window size varies)
				if (abs(target.x) < SNAP) {
					target.w = target.w + target.x;
					target.x = 0;
				}
				if (abs(target.y) < SNAP) {
					target.h = target.h + target.y;
					target.y = 0;
				}
				if (abs(root.width - target.x - target.w) < SNAP)
					target.w = root.width - target.x;
				if (abs(root.height - target.y - target.h) < SNAP)
					target.h = root.height - target.y;
			}

			XMoveResizeWindow(display, event.xmotion.window,
				target.x, target.y, target.w, target.h
			);
			break;
		case ButtonRelease:
			XUngrabPointer(display, event.xbutton.time);
			break;
		case MappingNotify: // display start menu?
		case CreateNotify: // fall through
		case UnmapNotify:
		case KeyRelease:
		case ConfigureNotify:
		case PropertyNotify:
			break;
		default:
			printf("UntrackedEvent%d\n", event.type);
		}
	};

}

int main (int argc, char **argv, char **envp) {
#ifdef DEBUG
	enable_debug();
#endif

	// save this for signal handlers
	argv0 = argv[0];
	// get signal reports
	report_signals();
	// don't die when X11 has a panic attack
	XSetErrorHandler(death_proof);

	// play stupid X games
	Display *display;
	if (!(display = XOpenDisplay(NULL))) {
		unsigned i = 0;
		while (envp[i] && strncmp(envp[i], "DISPLAY=:", 9)) i++;
		fprintf(stderr, "Can't open display \"%s\"\n", envp[i] + 9);
		return 1;
	}

	// get a handle on the root window
	root.handle = DefaultRootWindow(display);
	XGetGeometry(display, root.handle, VOID, VOID, VOID, &root.width,
		&root.height, VOID, VOID);

	// cache atoms
	initialize_atom_cache(display, envp);

	int screen = DefaultScreen(display);
	XVisualInfo visualinfo = { 0 };
	XMatchVisualInfo(display, screen, BPP, TrueColor, &visualinfo);

	// create a window to hold the window manager selection
	pede = XCreateWindow(display, root.handle,
#if defined(LEFT) && defined(TOP)
		LEFT, TOP,
#elif defined(RIGHT) && defined(TOP)
		root.width - WIDTH - RIGHT, TOP,
#elif defined(LEFT) && defined(BOTTOM)
		LEFT, root.height - HEIGHT - BOTTOM,
#elif defined(RIGHT) && defined(BOTTOM)
		root.width - WIDTH - RIGHT,
		root.height - HEIGHT - BOTTOM,
#else
#error "I don't know where you want the button, man..."
#endif
		WIDTH, HEIGHT, 0, visualinfo.depth, InputOutput, visualinfo.visual,
		CWColormap | CWEventMask | CWBackPixmap | CWBorderPixel,
		&(XSetWindowAttributes){
			.colormap = XCreateColormap(display, root.handle,
				visualinfo.visual, AllocNone),
			.event_mask = ExposureMask | ButtonPressMask,
			.background_pixmap = None, .border_pixel = 0
		}
	);

	pid_t pid = getpid();
	XChangeProperty(display, pede, XInternAtom(display, "_NET_WM_PID", False),
		atom[CARDINAL], 32, PropModeReplace, (void *)&pid, 1);
	XChangeProperty(display, pede, XInternAtom(display, "WM_CLASS", False),
		atom[UTF8_STRING], 8, PropModeReplace, (void *)"pede", 4);
	char *title = "Pegasus Epsilon's Desktop Environment";
	XChangeProperty(display, pede, XInternAtom(display, "_NET_WM_NAME", False),
		atom[UTF8_STRING], 8, PropModeReplace, (void *)title, strlen(title));
	XStoreName(display, pede, title);
	XChangeProperty(display, pede, atom[_NET_SUPPORTING_WM_CHECK],
		atom[WINDOW], 32, PropModeReplace, (void *)&pede, 1);

	GC gc = XCreateGC(display, pede, 0, 0);
	// we assume FILENAME lives in our directory
	char *filename;
	filename = malloc(1 + strlen(argv[0]));
	strcpy(filename, argv[0]);
	dirname(filename);
	filename = realloc(filename, 2 + strlen(filename) + strlen(FILENAME));
	strcat(strcat(filename, "/"), FILENAME);
	XImage *img = load_image(display, screen, visualinfo.visual, filename);
	// load_image frees the filename for us
	// free(filename);

	become_wm(display, pede);

	// set up root window
	XChangeProperty(display, root.handle, atom[_NET_SUPPORTING_WM_CHECK],
		atom[WINDOW], 32, PropModeReplace, (void *)&pede, 1);
	XChangeProperty(display, root.handle,
		XInternAtom(display, "_NET_SUPPORTED", False), atom[ATOM], 32,
		PropModeReplace, (void *)atom, sizeof(atom) / sizeof(Atom));
	XChangeProperty(display, root.handle,
		XInternAtom(display, "_NET_NUMBER_OF_DESKTOPS", False),
		atom[CARDINAL], 32, PropModeReplace, (void *)"\4\0\0\0", 1);
	// FIXME: EWMH this should contain an actual managed window list.
	XChangeProperty(display, root.handle,
		XInternAtom(display, "_NET_CLIENT_LIST", False), atom[ATOM],
		32, PropModeReplace, (void *)&pede, 1);

	// subscribe to events we actually care about
	// if the WM being replaced destroys their input selection holding window
	// before deselecting SubstructureRedirectMask, we will get BadAccess here.
	// This race condition is documented in ICCCM. That is their bug, not ours.
	XSelectInput(display, root.handle,
		SubstructureNotifyMask // CirculateNotify
		| SubstructureRedirectMask // MapRequest
//		| PropertyChangeMask // PropertyNotify
	);

	keycodes[KcTab] = XKeysymToKeycode(display, XStringToKeysym("Tab"));
	keycodes[KcF4] = XKeysymToKeycode(display, XStringToKeysym("F4"));
	keycodes[KcR] = XKeysymToKeycode(display, XStringToKeysym("R"));
	keycodes[Kc1] = XKeysymToKeycode(display, XStringToKeysym("1"));
	keycodes[Kc2] = XKeysymToKeycode(display, XStringToKeysym("2"));
	keycodes[Kc3] = XKeysymToKeycode(display, XStringToKeysym("3"));
	keycodes[Kc4] = XKeysymToKeycode(display, XStringToKeysym("4"));
	XGrabKey(display, keycodes[KcTab], Mod1Mask, root.handle, True, GrabModeAsync, GrabModeAsync);
	XGrabKey(display, keycodes[KcTab], Mod1Mask | ShiftMask, root.handle, True, GrabModeAsync, GrabModeAsync);
	XGrabKey(display, keycodes[KcF4], Mod1Mask, root.handle, True, GrabModeAsync, GrabModeAsync);
	XGrabKey(display, keycodes[KcR], Mod4Mask, root.handle, True, GrabModeAsync, GrabModeAsync);
	XGrabKey(display, keycodes[Kc1], Mod4Mask, root.handle, True, GrabModeAsync, GrabModeAsync);
	XGrabKey(display, keycodes[Kc2], Mod4Mask, root.handle, True, GrabModeAsync, GrabModeAsync);
	XGrabKey(display, keycodes[Kc3], Mod4Mask, root.handle, True, GrabModeAsync, GrabModeAsync);
	XGrabKey(display, keycodes[Kc4], Mod4Mask, root.handle, True, GrabModeAsync, GrabModeAsync);
	XGrabKey(display, keycodes[Kc1], Mod4Mask | ShiftMask, root.handle, True, GrabModeAsync, GrabModeAsync);
	XGrabKey(display, keycodes[Kc2], Mod4Mask | ShiftMask, root.handle, True, GrabModeAsync, GrabModeAsync);
	XGrabKey(display, keycodes[Kc3], Mod4Mask | ShiftMask, root.handle, True, GrabModeAsync, GrabModeAsync);
	XGrabKey(display, keycodes[Kc4], Mod4Mask | ShiftMask, root.handle, True, GrabModeAsync, GrabModeAsync);

	// left click = raise window
	XGrabButton(display, 1, 0, root.handle, True, ButtonPressMask,
			GrabModeSync, GrabModeSync, None, None);
	// super+left = move window
	XGrabButton(display, 9, 0, root.handle, True, ButtonPressMask,
			GrabModeAsync, GrabModeAsync, None, None);
	XGrabButton(display, 1, Mod4Mask, root.handle, True, ButtonPressMask,
			GrabModeAsync, GrabModeAsync, None, None);
	// super+right = resize window
	XGrabButton(display, 3, Mod4Mask, root.handle, True, ButtonPressMask,
			GrabModeAsync, GrabModeAsync, None, None);

	activate_workspace(display, 0);
	XMapWindow(display, pede);

	event_loop(display, pede, gc, img);

	// show all windows
	activate_workspace(display, (uint32_t)-1);

	XDestroyImage(img);
	XFreeGC(display, gc);
	XDestroyWindow(display, pede);
	XCloseDisplay(display);

	shutdown_atom_cache();
	puts("Clean shutdown.");

	return 0;
}
