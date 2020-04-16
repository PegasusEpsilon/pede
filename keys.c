#include <X11/Xlib.h>
#include <unistd.h>
#include <stdint.h>
#include <stdio.h>

#include "wm.h"
#include "config.h"

typedef enum {
#define KEY_EXPANDO(x) Kc ## x,
#include "expandos.h"
	Kc_LAST
} WatchedKeyCodes;
static KeyCode keycodes[Kc_LAST];
static char *keycode_names[] = {
#define KEY_EXPANDO(x) [Kc ## x] = #x,
#include "expandos.h"
};

void handle_key_events (XEvent event) {
	switch(event.type) {
	case KeyPress:
		// stupid C rules about switch blocks requiring integer constants...
		if (event.xkey.keycode == keycodes[KcTab]) {//do {
			// TODO: change this to manually select and raise
			// the bottom window or lower the top window
			XCirculateSubwindows(event.xkey.display, event.xkey.root,
				event.xkey.state & Mod1Mask ? LowerHighest : RaiseLowest);
		}// while (active_window(event.xkey.display) == pede);
		else if (event.xkey.keycode == keycodes[KcR]) {
			if (!fork()) execlp(RUNNER, RUNNER, RUNNERARGS, NULL);
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
		} else if (event.xkey.keycode == keycodes[KcXF86AudioRaiseVolume]) {
			if (!fork())
				execlp(VOLUME_CONTROL, VOLUME_CONTROL, RAISE_VOLUME, NULL);
		} else if (event.xkey.keycode == keycodes[KcXF86AudioLowerVolume]) {
			if (!fork())
				execlp(VOLUME_CONTROL, VOLUME_CONTROL, LOWER_VOLUME, NULL);
		} else if (event.xkey.keycode == keycodes[KcXF86AudioMute]) {
			if (!fork())
				execlp(VOLUME_CONTROL, VOLUME_CONTROL, TOGGLE_MUTE, NULL);
		} else puts("caught unhandled keystroke, your KEY_EXPANDO list "
				"is out of sync with keys.c");
		break;
	case KeyRelease:
		if (keycodes[KcAlt_L] == event.xkey.keycode)
			XLowerWindow(event.xkey.display, pede);
		break;
	}
}

void hook_keys (Display *display, Window root) {
#define KEY_EXPANDO(x) keycodes[Kc ## x] = XKeysymToKeycode(display, XStringToKeysym(keycode_names[Kc ## x]));
#include "expandos.h"
	XGrabKey(display, keycodes[KcXF86AudioRaiseVolume], None, root, True, GrabModeAsync, GrabModeAsync);
	XGrabKey(display, keycodes[KcXF86AudioLowerVolume], None, root, True, GrabModeAsync, GrabModeAsync);
	XGrabKey(display, keycodes[KcXF86AudioMute], None, root, True, GrabModeAsync, GrabModeAsync);
	XGrabKey(display, keycodes[KcAlt_L], None, root, True, GrabModeAsync, GrabModeAsync);
	XGrabKey(display, keycodes[KcTab], Mod1Mask, root, True, GrabModeAsync, GrabModeAsync);
	XGrabKey(display, keycodes[KcTab], Mod1Mask | ShiftMask, root, True, GrabModeAsync, GrabModeAsync);
	XGrabKey(display, keycodes[KcF4], Mod1Mask, root, True, GrabModeAsync, GrabModeAsync);
	XGrabKey(display, keycodes[KcR], Mod4Mask, root, True, GrabModeAsync, GrabModeAsync);
	XGrabKey(display, keycodes[Kc1], Mod4Mask, root, True, GrabModeAsync, GrabModeAsync);
	XGrabKey(display, keycodes[Kc2], Mod4Mask, root, True, GrabModeAsync, GrabModeAsync);
	XGrabKey(display, keycodes[Kc3], Mod4Mask, root, True, GrabModeAsync, GrabModeAsync);
	XGrabKey(display, keycodes[Kc4], Mod4Mask, root, True, GrabModeAsync, GrabModeAsync);
	XGrabKey(display, keycodes[Kc1], Mod4Mask | ShiftMask, root, True, GrabModeAsync, GrabModeAsync);
	XGrabKey(display, keycodes[Kc2], Mod4Mask | ShiftMask, root, True, GrabModeAsync, GrabModeAsync);
	XGrabKey(display, keycodes[Kc3], Mod4Mask | ShiftMask, root, True, GrabModeAsync, GrabModeAsync);
	XGrabKey(display, keycodes[Kc4], Mod4Mask | ShiftMask, root, True, GrabModeAsync, GrabModeAsync);
}
