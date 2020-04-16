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

#include "wm_core.h"
#include "keys.h"
#include "atoms.h"
#include "events.h"
#include "signal_events.h"
#include "config.h"

// TODO: break everything out into modules, then write:
// inter-window snapping/gluing, generalized keyboard shortcuts
// better alt-tab, fullscreen support, window restoration after fullscreen,
// vastly improve ICCCM and EWMH support...
// tiling module? windows 10-style edge tiling?

#define MAX(a, b) ((a) > (b) ? (a) : (b))

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

int death_proof (Display *display, XErrorEvent *error) {
	XGetErrorText(display, error->error_code, buffer, BUFFER_LENGTH-1);
	printf("X request %d: Error %d/%d.%d: %s\n", error->serial, error->error_code,
		error->request_code, error->minor_code, buffer);
	fflush(stdout);
	return 0;
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

char *argv0 = NULL;
bool signal_handler (void) {
	printf("received signal %d\n", which_signal);
	fflush(stdout);
	switch (which_signal) {
	case SIGINT:
		puts("\nInterrupted.");
		return True;
	case SIGTERM:
		puts("\nTerminated.");
		return True;
	case SIGUSR1:
		execlp(argv0, argv0);
		// we never get here anyway
	}
	return false;
}

Bool XWaitEvent (Display *display) {
	int Xfd = ConnectionNumber(display);
	fd_set listen, pending;
	FD_ZERO(&listen);
	FD_SET(Xfd, &listen);
	while (!XPending(display)) {
		pending = listen;
		if (0 >= select(FD_SETSIZE, &pending, NULL, NULL, NULL))
			if (signal_handler()) return True;
	}
	return False;
}

void event_loop (Display *display, Window pede, GC gc, XImage *img) {
	XEvent event;
	XWindowAttributes attrStart;
	XButtonEvent dragStart = { 0 };
	char moveSide = 0;
	while (!XWaitEvent(display)) {
		if (!XPending(display)) return;
		XNextEvent(display, &event);
		printf("event %s next request %d\n", event_names[event.type], XNextRequest(display)); fflush(stdout);
		handle_key_events(event);
		switch (event.type) {
		case MapNotify:
			if (event.xmaprequest.window == pede) {
				puts("I have arrived.");
				XLowerWindow(display, pede);
			}
			break;
		case ConfigureRequest:
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
			// window requests focus
			else if (event.xclient.message_type == atom[_NET_ACTIVE_WINDOW]) {
				unsigned long count = 0;
				unsigned char *prop = NULL;
				XGetWindowProperty(event.xclient.display, event.xclient.window,
					atom[_NET_WM_DESKTOP], 0, 1, False, atom[CARDINAL],
					VOID, VOID, &count, VOID, &prop);
				if (count) {
					activate_workspace(event.xclient.display, *prop);
					XRaiseWindow(event.xclient.display, event.xclient.window);
					focus_active_window(event.xclient.display);
				}
				XFree(prop);
			} else // idfk...
				printf("Received unmonitored client message: %s/%d\n",
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
		case ConfigureNotify:
		case PropertyNotify:
		case KeyRelease:
		case KeyPress:
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

	hook_keys(display, root.handle);

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

	activate_workspace(display, active_workspace(display));
	XMapWindow(display, pede);

	// actually do the thing
	event_loop(display, pede, gc, img);
	// done doing the thing, shut it down, shut it down forever

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
