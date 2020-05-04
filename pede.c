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
#include <stdint.h> 	// uint32_t

#include <libgen.h> 	// dirname()
#include <string.h> 	// strlen()
#include <signal.h> 	// SIGINT, SIGCHLD, SIGTERM, SIGUSR1
#include <sys/select.h>	// fd_set, pipe(), FD_SET(), FD_ZERO(), select()
#include <sys/wait.h>	// wait()
#include <time.h>   	// nanosleep()

#include "config.h"
#include "defines.h"
#include "types.h"
#include "util.h"
#include "keys.h"
#include "atoms.h"
#include "events.h"
#include "signal_events.h"
#include "wm_core.h"
#include "move_modifiers.h"
#include "size_modifiers.h"

// XGetErrorText opens an XrmDatabase connection, closes it, allocates memory,
// frees it, and leaks about a meg and a half somewhere along the way. It only
// leaks once, the first time you call it, but we don't need to burn that RAM
// anyway. We can do better.
static const char *_XErrorList[] = {
	"noerror", "BadRequest", "BadValue", "BadWindow", "BadPixmap", "BadAtom",
	"BadCursor", "BadFont", "BadMatch", "BadDrawable", "BadAccess", "BadAlloc",
	"BadColor", "BadGC", "BadIDChoice", "BadName", "BadLength",
	"BadImplementation", "unknownerror"
};
int death_proof (Display *display, XErrorEvent *error) {
	if (18 < error->error_code) error->error_code = 18;
	printf("X request %d: Error %d/%d.%d: %s\n", error->serial,
		error->error_code, error->request_code, error->minor_code,
		_XErrorList[error->error_code]);
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
	struct { uint8_t r, g, b, a; } c;
	uint32_t packed;
} __attribute__((packed)) PIXEL32;

XImage *load_image (Display *display, int screen, Visual *visual, char *filename) {
	PIXEL32 *data = malloc(WIDTH * HEIGHT * sizeof(PIXEL32));

	FILE *file = fopen(filename, "r");
	free(filename);
	if (!file) {
		perror("failed opening " FILENAME);
		fflush(stdout);
		exit(1);
	}
	fread(data, sizeof(PIXEL32), WIDTH * HEIGHT, file);
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
		(void *)data, WIDTH, HEIGHT, BPP, WIDTH * sizeof(PIXEL32));
}

XImage *img;
GC gc;
void cleanup (void) {
	unhook_keys();
	XDestroyImage(img);
	XFreeGC(display, gc);
	XDestroyWindow(display, pede);
	XCloseDisplay(display);
	shutdown_atom_cache();
}

char *argv0 = NULL;
Bool signal_handler (void) {
	printf("event SIG%s\n", signame(which_signal));
	fflush(stdout);
	switch (which_signal) {
	case SIGHUP:
		return True;
	case SIGINT:
		puts("Interrupted.");
		return True;
	case SIGUSR1:
		cleanup();
		execlp(argv0, argv0, NULL);
		// we never get here anyway
	case SIGTERM:
		puts("Terminated.");
		return True;
	case SIGCHLD: {
			int status;
			while (0 < waitpid(-1, &status, WNOHANG))
				printf("Child process returned %d.\n", status);
		};
		break;
	}
	return False;
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
	BOX windowStart;
	CLICK mouseStart = (CLICK){ .btn = -1 };
	char moveSide = 0;
	while (!XWaitEvent(display)) {
		if (!XPending(display)) return;
		XNextEvent(display, &event);
		printf("event %s next request %d\n", event_names[event.type], XNextRequest(display)); fflush(stdout);
		if (KeyPress == event.type || KeyRelease == event.type)
			handle_key_events(event);
		else switch (event.type) {
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
				}
			);
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
				activate_workspace(*event.xclient.data.l);
			// move window to given workspace
			else if (event.xclient.message_type == atom[_NET_WM_DESKTOP])
				set_workspace(event.xclient.window, *event.xclient.data.l);
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
					activate_workspace(*prop);
					XRaiseWindow(event.xclient.display, event.xclient.window);
					focus_active_window();
				}
				XFree(prop);
			} else {
				// idfk...
				char *state_name = XGetAtomName(display,
					event.xclient.message_type);
				printf("Received unmonitored client message: %s/%d\n",
					state_name, *event.xclient.data.l);
				XFree(state_name);
			}
			break;
		case MapRequest:
			map_window(&event.xmaprequest);
			set_workspace(event.xmaprequest.window, active_workspace());
			focus_window(event.xmaprequest.window);
			break;
		case DestroyNotify: // fall through
		case CirculateNotify:
			focus_active_window();
			break;
		case SelectionClear:
			if (event.xselectionclear.selection == atom[WM_Sn]) {
				puts("Lost window manager status, cleaning up...");
				return;
			}
			break;
		case ButtonPress:
			// always replay clicks to the root window
			if (None == event.xbutton.subwindow) {
				XAllowEvents(display, ReplayPointer, event.xbutton.time);
				break;
			}
			if (pede == event.xbutton.subwindow) return;
			XRaiseWindow(display, event.xbutton.subwindow);
			XSetInputFocus(display, event.xbutton.subwindow,
				RevertToParent, event.xbutton.time);
			if (!(event.xbutton.state & ~Mod2Mask)
				&& Button9 != event.xbutton.button) {
				XAllowEvents(display, ReplayPointer, event.xbutton.time);
				break;
			}

			// event.xbutton.x and event.xbutton.y are not consistent
			mouseStart = (CLICK){
				.x = event.xbutton.x_root,
				.y = event.xbutton.y_root,
				.btn = event.xbutton.button
			};
			XGetGeometry(display, event.xbutton.subwindow, VOID,
				&windowStart.x, &windowStart.y, &windowStart.w, &windowStart.h,
				VOID, VOID);
			XGrabPointer(event.xbutton.display, event.xbutton.subwindow,
				True, PointerMotionMask | ButtonReleaseMask,
				GrabModeAsync, GrabModeAsync, None, None, event.xbutton.time);

			if (Button1 == event.xbutton.button ||
				Button9 == event.xbutton.button) break; // move

			int relative_x = mouseStart.x - windowStart.x;
			int relative_y = mouseStart.y - windowStart.y;

			moveSide = // calculate which nonant the click occurred in
				((relative_y < 2 * windowStart.h / 3) << SIDE_TOP_BIT) |
				((relative_x > windowStart.w / 3) << SIDE_RIGHT_BIT) |
				((relative_y > windowStart.h / 3) << SIDE_BOTTOM_BIT) |
				((relative_x < 2 * windowStart.w / 3) << SIDE_LEFT_BIT);

			// if middle side nonant clicked, move only that side
			// I feel like there's better code for this...
			switch (moveSide) {
			case SIDE_LEFT_MASK | SIDE_TOP_MASK | SIDE_RIGHT_MASK:
				moveSide = SIDE_TOP_MASK;
				break;
			case SIDE_TOP_MASK | SIDE_RIGHT_MASK | SIDE_BOTTOM_MASK:
				moveSide = SIDE_RIGHT_MASK;
				break;
			case SIDE_RIGHT_MASK | SIDE_BOTTOM_MASK | SIDE_LEFT_MASK:
				moveSide = SIDE_BOTTOM_MASK;
				break;
			case SIDE_BOTTOM_MASK | SIDE_LEFT_MASK | SIDE_TOP_MASK:
				moveSide = SIDE_LEFT_MASK;
				break;
			}
			break;
		case MotionNotify:
			while (XCheckTypedEvent(display, MotionNotify, &event));
			int xdiff = event.xbutton.x_root - mouseStart.x;
			int ydiff = event.xbutton.y_root - mouseStart.y;
			BOX target = windowStart;

			if (Button1 == mouseStart.btn || Button9 == mouseStart.btn) {
				// move
				target.x = windowStart.x + xdiff;
				target.y = windowStart.y + ydiff;

				for (unsigned i = 0; i < move_modifiers_length; i++)
					move_modifiers[i](event.xmotion.window, &target);
			} else {
				// if center nonant clicked, always grow first
				if (SIDE_CENTER(moveSide)) {
					moveSide =
						((ydiff < 0) << SIDE_TOP_BIT) |
						((xdiff > 0) << SIDE_RIGHT_BIT) |
						((ydiff > 0) << SIDE_BOTTOM_BIT) |
						((xdiff < 0) << SIDE_LEFT_BIT);
					int xmag = abs(xdiff);
					int ymag = abs(ydiff);
					if (xmag / 2 > ymag)
						moveSide &= SIDE_TOP_MASK | SIDE_BOTTOM_MASK;
					if (ymag / 2 > xmag)
						moveSide &= SIDE_LEFT_MASK | SIDE_RIGHT_MASK;
				}

				if (SIDE_TOP(moveSide)) {
					target.h = MAX(MINIMUM_SIZE, (int)windowStart.h - ydiff);
					target.y -= target.h - windowStart.h;
				} else if (SIDE_BOTTOM(moveSide))
					target.h = MAX(MINIMUM_SIZE, (int)windowStart.h + ydiff);

				if (SIDE_LEFT(moveSide)) {
					target.w = MAX(MINIMUM_SIZE, (int)windowStart.w - xdiff);
					target.x -= target.w - windowStart.w;
				} else if (SIDE_RIGHT(moveSide))
					target.w = MAX(MINIMUM_SIZE, (int)windowStart.w + xdiff);

				for (unsigned i = 0; i < size_modifiers_length; i++)
					size_modifiers[i](event.xmotion.window, moveSide, &target);
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
			break;
		default:
			printf("UntrackedEvent%d\n", event.type);
		}
	};
}

static void numlock_doesnt_matter (unsigned btn, unsigned mod) {
	XGrabButton(display, btn,            mod, root.handle, True,
			ButtonPressMask, GrabModeSync, GrabModeSync, None, None);
	XGrabButton(display, btn, Mod2Mask | mod, root.handle, True,
			ButtonPressMask, GrabModeSync, GrabModeSync, None, None);
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
	XChangeProperty(display, pede, atom[_NET_WM_WINDOW_TYPE], atom[ATOM], 32,
		PropModeReplace, (void *)&atom[_NET_WM_WINDOW_TYPE_DESKTOP], 1);
	XStoreName(display, pede, title);
	XChangeProperty(display, pede, atom[_NET_SUPPORTING_WM_CHECK],
		atom[WINDOW], 32, PropModeReplace, (void *)&pede, 1);

	gc = XCreateGC(display, pede, 0, 0);
	// we assume FILENAME lives in our directory
	char *filename;
	filename = malloc(1 + strlen(argv[0]));
	strcpy(filename, argv[0]);
	dirname(filename);
	filename = realloc(filename, 2 + strlen(filename) + strlen(FILENAME));
	strcat(strcat(filename, "/"), FILENAME);
	img = load_image(display, screen, visualinfo.visual, filename);
	// load_image frees the filename for us
	// free(filename);

	make_wm(pede);

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
		XInternAtom(display, "_NET_CLIENT_LIST", False), atom[WINDOW],
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

	hook_keys();

	// Button9 (my thumb button) = move window
	//numlock_doesnt_matter(Button9, None);
	// left click = raise window
	numlock_doesnt_matter(Button1, None);
	// super+left = move window
	numlock_doesnt_matter(Button1, Mod4Mask);
	// super+right = resize window
	numlock_doesnt_matter(Button3, Mod4Mask);

	activate_workspace(active_workspace());
	XMapWindow(display, pede);

	// actually do the thing
	event_loop(display, pede, gc, img);
	// done doing the thing, shut it down, shut it down forever

	// show all windows
	activate_workspace((uint32_t)-1);

	cleanup();

	puts("Clean shutdown.");

	return 0;
}
