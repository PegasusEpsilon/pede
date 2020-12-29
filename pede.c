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
#include <sys/wait.h>	// waitpid()
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
int death_proof (Display *ignored, XErrorEvent *error) {
	ignored = ignored; // stfu gcc
	if (18 < error->error_code) error->error_code = 18;
	printf("X request %d: Error %d/%d.%d: %s\n", error->serial,
		error->error_code, error->request_code, error->minor_code,
		_XErrorList[error->error_code]);
	fflush(stdout);
	return 0;
}

void set_window_name (Window w, char *name) {
	XTextProperty textprop;
	XStringListToTextProperty(&name, 1, &textprop);
	XStoreName(display, w, name);
	XSetWMName(display, w, &textprop);
	XChangeProperty(display, w, XInternAtom(display, "_NET_WM_NAME", False),
		atom[UTF8_STRING], 8, PropModeReplace,
		(unsigned char *)name, (int)strlen(name));
}

typedef union {
	struct { uint8_t r, g, b, a; } c;
	uint32_t packed;
} __attribute__((packed)) PIXEL32;

XImage *load_image (Visual *visual, char *filename) {
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
		// something has gone horribly wrong if this runs
		puts("I can't execute myself. My binary has probably gone missing.");
		puts("I will now crash. Say goodbye to your X session!");
		exit(-1);
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

Bool XWaitEvent (void) {
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

void event_loop (void) {
	XEvent event;
	BOX windowStart;
	CLICK mouseStart = (CLICK){ .btn = (unsigned)-1 };
	char moveSide = 0;
	while (!XWaitEvent()) {
		if (!XPending(display)) return;
		XNextEvent(display, &event);
		if (KeyPress == event.type || KeyRelease == event.type)
			handle_key_events(event);
		else switch (event.type) {
		case MapNotify:
			if (event.xmaprequest.window == pede) {
				puts("I have arrived.");
				fflush(stdout);
				XLowerWindow(display, pede);
			} else {
				window_diagnostic("Window ", event.xmaprequest.window,
					" has been mapped\n");
				// FIXME: do not raise if keypress within last N second(s)?
				XRaiseWindow(event.xmaprequest.display,
					event.xmaprequest.window);
				focus_window(event.xmaprequest.window);
			}
			break;
		case ConfigureRequest:
			XConfigureWindow(
				event.xconfigurerequest.display,
				event.xconfigurerequest.window,
				(unsigned)event.xconfigurerequest.value_mask,
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
			break;
		case Expose:
			XPutImage(display, pede, gc, img, 0, 0, 0, 0, WIDTH, HEIGHT);
			break;
		case SelectionRequest: // ICCCM compliance, request always refused
			refuse_selection_request(event.xselectionrequest);
			break;
		case ClientMessage:
			// switch to given workspace
			if (event.xclient.message_type == atom[_NET_CURRENT_DESKTOP])
				activate_workspace((Workspace)*event.xclient.data.l);
			// move window to given workspace
			else if (event.xclient.message_type == atom[_NET_WM_DESKTOP])
				set_workspace(event.xclient.window,
					(Workspace)*event.xclient.data.l);
			// add or remove various states to _NET_WM_STATE properties
			else if (event.xclient.message_type == atom[_NET_WM_STATE])
				alter_window_state(event.xclient);
			// window requests focus
			else if (event.xclient.message_type == atom[_NET_ACTIVE_WINDOW]) {
				Workspace *workspace = NULL;
				XGetWindowProperty(event.xclient.display, event.xclient.window,
					atom[_NET_WM_DESKTOP], 0, 1, False, atom[CARDINAL],
					VOID, VOID, VOID, VOID, (void *)&workspace);
				if (workspace) {
					activate_workspace(*workspace);
					XRaiseWindow(event.xclient.display, event.xclient.window);
					focus_window(event.xclient.window);
					XFree(workspace);
				}
			} else {
				// idfk...
				char *state_name = XGetAtomName(display,
					event.xclient.message_type);
				printf("Received unmonitored client message: %s/%d\n",
					state_name, *event.xclient.data.l);
				XFree(state_name);
			}
			break;
		case MapRequest: {
			map_window(&event.xmaprequest);
			set_workspace(event.xmaprequest.window, active_workspace());
			focus_window(event.xmaprequest.window);
			Window *list;
			long unsigned count = get_window_property_array(root.handle,
				atom[_NET_CLIENT_LIST], atom[WINDOW], (void **)&list);
			Window *new_list = realloc(list, ++count * sizeof(Window));
			if (new_list) {
				list = new_list;
				list[count - 1] = event.xmaprequest.window;
			}
			XChangeProperty(display, root.handle, atom[_NET_CLIENT_LIST],
				atom[WINDOW], 32, PropModeReplace, (void *)list, (int)count);
			XFree(list);
			window_diagnostic("Window ", event.xmaprequest.window,
				" requests mapping\n");
			break;
		};
		case DestroyNotify:
			// Can't get title of a destroyed window, so don't do window_diagnostic
			printf("Window 0x%08x has been destroyed.\n", event.xdestroywindow.window);
			// fall through
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
			focus_window(event.xbutton.subwindow);
			XSync(display, False);
			if (!(event.xbutton.state & Mod4Mask)
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
				&windowStart.pos.x, &windowStart.pos.y, &windowStart.size.w,
				&windowStart.size.h, VOID, VOID);
			XGrabPointer(event.xbutton.display, event.xbutton.subwindow,
				True, PointerMotionMask | ButtonReleaseMask,
				GrabModeAsync, GrabModeAsync, None, None, event.xbutton.time);

			if (Button1 == event.xbutton.button ||
				Button9 == event.xbutton.button) break; // move

			int relative_x = mouseStart.x - windowStart.pos.x;
			int relative_y = mouseStart.y - windowStart.pos.y;

			moveSide = (char)( // calculate which nonant the click occurred in
				((relative_y < (int)(2 * windowStart.size.h / 3)) << SIDE_TOP_BIT) |
				((relative_x > (int)(windowStart.size.w / 3)) << SIDE_RIGHT_BIT) |
				((relative_y > (int)(windowStart.size.h / 3)) << SIDE_BOTTOM_BIT) |
				((relative_x < (int)(2 * windowStart.size.w / 3)) << SIDE_LEFT_BIT)
			);

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
				target.pos.x = windowStart.pos.x + xdiff;
				target.pos.y = windowStart.pos.y + ydiff;

				for (unsigned i = 0; i < move_modifiers_length; i++)
					move_modifiers[i](event.xmotion.window, &target);
			} else {
				// if center nonant clicked, always grow first
				if (SIDE_CENTER(moveSide)) {
					moveSide = (char)(
						((ydiff < 0) << SIDE_TOP_BIT) |
						((xdiff > 0) << SIDE_RIGHT_BIT) |
						((ydiff > 0) << SIDE_BOTTOM_BIT) |
						((xdiff < 0) << SIDE_LEFT_BIT)
					);
					int xmag = abs(xdiff);
					int ymag = abs(ydiff);
					if (xmag / 2 > ymag)
						moveSide &= SIDE_TOP_MASK | SIDE_BOTTOM_MASK;
					if (ymag / 2 > xmag)
						moveSide &= SIDE_LEFT_MASK | SIDE_RIGHT_MASK;
				}

				if (SIDE_TOP(moveSide)) {
					target.size.h = MAX(
						MINIMUM_SIZE, windowStart.size.h - (unsigned)ydiff
					);
					target.pos.y -= (int)(target.size.h - windowStart.size.h);
				} else if (SIDE_BOTTOM(moveSide))
					target.size.h = MAX(
						MINIMUM_SIZE, windowStart.size.h + (unsigned)ydiff
					);

				if (SIDE_LEFT(moveSide)) {
					target.size.w = MAX(
						MINIMUM_SIZE, windowStart.size.w - (unsigned)xdiff
					);
					target.pos.x -= (int)(target.size.w - windowStart.size.w);
				} else if (SIDE_RIGHT(moveSide))
					target.size.w = MAX(
						MINIMUM_SIZE, windowStart.size.w + (unsigned)xdiff
					);

				for (unsigned i = 0; i < size_modifiers_length; i++)
					size_modifiers[i](event.xmotion.window, moveSide, &target);
			}

			XMoveResizeWindow(
				display, event.xmotion.window,
				target.pos.x, target.pos.y, target.size.w, target.size.h
			);
			break;
		case ButtonRelease:
			XUngrabPointer(display, event.xbutton.time);
			break;
		case UnmapNotify: {
			Workspace ws = window_workspace(event.xunmap.window);
			if (-1 == ws || active_workspace() == ws) {
				window_diagnostic("window ", event.xunmap.window, " has been unmapped\n");
				Window *list;
				long unsigned count = get_window_property_array(root.handle,
					atom[_NET_CLIENT_LIST], atom[WINDOW], (void **)&list);
				count = delete_window_from_array(list, count, window_isnt(
					event.xunmap.window));
				XChangeProperty(display, root.handle, atom[_NET_CLIENT_LIST],
					atom[WINDOW], 32, PropModeReplace, (void *)list, (int)count);
				XFree(list);
				XDeleteProperty(display, event.xunmap.window,
					atom[_NET_WM_DESKTOP]);
			}
			break;
		};
		case MappingNotify: // display start menu?
		case CreateNotify: // fall through
		case ConfigureNotify:
		case PropertyNotify:
			break;
		default:
			printf("UntrackedEvent%d\n", event.type);
		}
		XSync(display, False);
	};
}

static void lock_ignoring_button_hook (unsigned btn, unsigned mod) {
	XGrabButton(display, btn,                       mod, root.handle, True,
			ButtonPressMask, GrabModeSync, GrabModeSync, None, None);
	XGrabButton(display, btn, LockMask | Mod2Mask | mod, root.handle, True,
			ButtonPressMask, GrabModeSync, GrabModeSync, None, None);
	XGrabButton(display, btn,            Mod2Mask | mod, root.handle, True,
			ButtonPressMask, GrabModeSync, GrabModeSync, None, None);
	XGrabButton(display, btn, LockMask |            mod, root.handle, True,
			ButtonPressMask, GrabModeSync, GrabModeSync, None, None);
}

int main (int argc, char **argv, char **envp) {
	// We only ever need argv[0]. If it doesn't exist, we'll just crash later.
	argc = argc;

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
		(int)root.width - WIDTH - RIGHT, TOP,
#elif defined(LEFT) && defined(BOTTOM)
		LEFT, (int)root.height - HEIGHT - BOTTOM,
#elif defined(RIGHT) && defined(BOTTOM)
		(int)root.width - WIDTH - RIGHT,
		(int)root.height - HEIGHT - BOTTOM,
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
		atom[UTF8_STRING], 8, PropModeReplace, (void *)title, (int)strlen(title));
	XChangeProperty(display, pede, atom[_NET_WM_WINDOW_TYPE], atom[ATOM], 32,
		PropModeReplace, (void *)&atom[_NET_WM_WINDOW_TYPE_DESKTOP], 1);
	XStoreName(display, pede, title);
	XChangeProperty(display, pede, atom[_NET_SUPPORTING_WM_CHECK],
		atom[WINDOW], 32, PropModeReplace, (void *)&pede, 1);
	set_sticky(pede);

	gc = XCreateGC(display, pede, 0, 0);
	// we assume FILENAME lives in our directory
	char *filename;
	filename = malloc(1 + strlen(argv[0]));
	strcpy(filename, argv[0]);
	dirname(filename);
	filename = realloc(filename, 2 + strlen(filename) + strlen(FILENAME));
	strcat(strcat(filename, "/"), FILENAME);
	img = load_image(visualinfo.visual, filename);
	// load_image frees the filename for us
	// free(filename);

	make_wm(pede);

	// set up root window
	XChangeProperty(display, root.handle, atom[_NET_SUPPORTING_WM_CHECK],
		atom[WINDOW], 32, PropModeReplace, (void *)&pede, 1);
	// the _NET_SUPPORTED property should only contain supported _NET_ atoms
	// currently we're just throwing all the atoms we know about into it
	// this is technically an error, but the spec is written in such a way
	// that no client is actually allowed to throw a fit about it
	XChangeProperty(display, root.handle,
		XInternAtom(display, "_NET_SUPPORTED", False), atom[ATOM], 32,
		PropModeReplace, (void *)atom, sizeof(atom) / sizeof(Atom));
	XChangeProperty(display, root.handle,
		XInternAtom(display, "_NET_NUMBER_OF_DESKTOPS", False),
		atom[CARDINAL], 32, PropModeReplace, (void *)"\4\0\0\0", 1);

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
	//lock_ignoring_button_hook(Button9, None);
	// left click = raise window
	lock_ignoring_button_hook(Button1, None);
	// super+left = move window
	lock_ignoring_button_hook(Button1, Mod4Mask);
	// super+right = resize window
	lock_ignoring_button_hook(Button3, Mod4Mask);

	activate_workspace(active_workspace());
	XMapWindow(display, pede);

	// actually do the thing
	event_loop();
	// done doing the thing, shut it down, shut it down forever

	// show all windows
	activate_workspace((uint32_t)-1);

	cleanup();

	puts("Clean shutdown.");

	return 0;
}
