#include <X11/Xlib.h>
#include <unistd.h>
#include <stdint.h>
#include <stdio.h>

#include "wm_core.h"
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
		if (event.xkey.keycode == keycodes[KcRight]) {
			activate_workspace((active_workspace() + 1) % 4);
		} else if (event.xkey.keycode == keycodes[KcLeft]) {
			activate_workspace((active_workspace() + 3) % 4);
		} else if (event.xkey.keycode == keycodes[KcTab]) {//do {
			// TODO: change this to manually select and raise
			// the bottom window or lower the top window
			XCirculateSubwindows(event.xkey.display, event.xkey.root,
				event.xkey.state & Mod1Mask ? LowerHighest : RaiseLowest);
		} else if (event.xkey.keycode == keycodes[KcR]) {
			if (!fork()) execlp(RUNNER, RUNNER, RUNNERARGS, NULL);
		} else if (event.xkey.keycode == keycodes[KcF4]) {
			close_window(active_window());
		} else if (event.xkey.keycode == keycodes[Kc1]) {
			if (event.xkey.state & ShiftMask)
				set_workspace(active_window(), 0);
			else activate_workspace(0);
		} else if (event.xkey.keycode == keycodes[Kc2]) {
			if (event.xkey.state & ShiftMask)
				set_workspace(active_window(), 1);
			else activate_workspace(1);
		} else if (event.xkey.keycode == keycodes[Kc3]) {
			if (event.xkey.state & ShiftMask)
				set_workspace(active_window(), 2);
			else activate_workspace(2);
		} else if (event.xkey.keycode == keycodes[Kc4]) {
			if (event.xkey.state & ShiftMask)
				set_workspace(active_window(), 3);
			else activate_workspace(3);
		} else if (event.xkey.keycode == keycodes[KcXF86AudioRaiseVolume]) {
			if (!fork())
				execlp(VOLUME_CONTROL, VOLUME_CONTROL, RAISE_VOLUME, NULL);
		} else if (event.xkey.keycode == keycodes[KcXF86AudioLowerVolume]) {
			if (!fork())
				execlp(VOLUME_CONTROL, VOLUME_CONTROL, LOWER_VOLUME, NULL);
		} else if (event.xkey.keycode == keycodes[KcXF86AudioMute]) {
			if (!fork())
				execlp(VOLUME_CONTROL, VOLUME_CONTROL, TOGGLE_MUTE, NULL);
		} else if (event.xkey.keycode == keycodes[KcAlt_L]) {
			// reserved for alt+tab
		} else {
			puts("caught unhandled keystroke, your KEY_EXPANDO list "
				"is out of sync with keys.c");
		}
		break;
	case KeyRelease:
		if (keycodes[KcAlt_L] == event.xkey.keycode)
			XLowerWindow(event.xkey.display, pede);
		break;
	}
}

static void numlock_doesnt_matter (int kc, unsigned mod) {
	XGrabKey(display, kc, Mod2Mask | mod, root.handle, True, GrabModeAsync, GrabModeAsync);
	XGrabKey(display, kc,            mod, root.handle, True, GrabModeAsync, GrabModeAsync);
}

void hook_keys (void) {
#define KEY_EXPANDO(x) keycodes[Kc ## x] = XKeysymToKeycode(display, XStringToKeysym(keycode_names[Kc ## x]));
#include "expandos.h"

	numlock_doesnt_matter(keycodes[KcXF86AudioRaiseVolume], None);
	numlock_doesnt_matter(keycodes[KcXF86AudioLowerVolume], None);
	numlock_doesnt_matter(keycodes[KcXF86AudioMute], None);
	numlock_doesnt_matter(keycodes[KcAlt_L], None);
	numlock_doesnt_matter(keycodes[KcRight], Mod4Mask);
	numlock_doesnt_matter(keycodes[KcLeft], Mod4Mask);
	numlock_doesnt_matter(keycodes[KcTab], Mod1Mask | ShiftMask);
	numlock_doesnt_matter(keycodes[KcTab], Mod1Mask);
	numlock_doesnt_matter(keycodes[KcF4], Mod1Mask);
	numlock_doesnt_matter(keycodes[KcR], Mod4Mask);
	numlock_doesnt_matter(keycodes[Kc1], Mod4Mask | ShiftMask);
	numlock_doesnt_matter(keycodes[Kc2], Mod4Mask | ShiftMask);
	numlock_doesnt_matter(keycodes[Kc3], Mod4Mask | ShiftMask);
	numlock_doesnt_matter(keycodes[Kc4], Mod4Mask | ShiftMask);
	numlock_doesnt_matter(keycodes[Kc1], Mod4Mask);
	numlock_doesnt_matter(keycodes[Kc2], Mod4Mask);
	numlock_doesnt_matter(keycodes[Kc3], Mod4Mask);
	numlock_doesnt_matter(keycodes[Kc4], Mod4Mask);
}
