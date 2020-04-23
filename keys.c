/* keys.c - Pegasus Epsilon's Desktop Environment
** Goal: Less than a megabyte resident
** (C)opyright 2019-2020 "Pegasus Epsilon" <pegasus@pimpninjas.org>
** Distribute Unmodified - https://pegasus.pimpninjas.org/license
*/

#define _XOPEN_SOURCE 500	// vfork()

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

static Window *window_list = NULL;
static unsigned focusing = 0, window_list_length = 0;
static void page_end (void) {
	XFree(window_list);
	focusing = window_list_length = 0;
	window_list = NULL;
}
static void page_previous (void) {
	// this is 100% wrong, but it doesn't crash, so good enough for now.
	// i'll keep pondering and come back to it.
	if (1 >= window_list_length) return;
	unsigned last = window_list_length - 1;
	if (focusing == 0) {
		XRaiseWindow(display, window_list[last]);
		page_end();
	} else {
		focusing--;
		window_list[last] ^= window_list[focusing];
		window_list[focusing] ^= window_list[last];
		window_list[last] ^= window_list[focusing];
		XRestackWindows(display, window_list, window_list_length);
	}
	focus_active_window();
}
static void page_next (void) {
	// this one works right, though...
	if (1 >= window_list_length) return;
	focusing++;
	if (focusing == window_list_length) {
		XLowerWindow(display, window_list[0]);
		XLowerWindow(display, pede);
		page_end();
	} else {
		window_list[0] ^= window_list[focusing];
		window_list[focusing] ^= window_list[0];
		window_list[0] ^= window_list[focusing];
		XRestackWindows(display, window_list, window_list_length);
	}
	focus_active_window();
}

// this macro will break if used in any manner other than just
// calling it as run("prog", "arg1", "arg2", ...);
// I can't really think of a way around it, but it shouldn't matter?
#define run(x, ...) if (!vfork()) execlp(x, x, __VA_ARGS__, NULL); \

static int prtscrn_state = None;
void handle_key_events (XEvent event) {
	switch(event.type) {
	case KeyPress:
		// stupid C rules about switch blocks requiring integer constants...
		if (
			event.xkey.keycode == keycodes[KcRight] &&
			event.xkey.state & Mod4Mask
		) {
			int workspace = (1 + active_workspace()) % 4;
			if (event.xkey.state & ShiftMask)
				set_workspace(active_window(), workspace);
			activate_workspace(workspace);
		} else if (event.xkey.keycode == keycodes[KcPrint]) {
			event.xkey.state &= ~Mod2Mask; // ignore numlock bit
			if (event.xkey.state == None) {
				run(PRTSCRN, PRTSCRN_ARGS);
			} else if (event.xkey.state == ControlMask) {
				prtscrn_state = event.xkey.state;
				XGrabKeyboard(display, root.handle, False,
					GrabModeAsync, GrabModeAsync, event.xkey.time);
				XSync(display, False);
			} else if (event.xkey.state == Mod4Mask) {
				prtscrn_state = event.xkey.state;
				XGrabKeyboard(display, root.handle, False,
					GrabModeAsync, GrabModeAsync, event.xkey.time);
				XSync(display, False);
			}
		} else if (
			event.xkey.keycode == keycodes[KcLeft] &&
			event.xkey.state & Mod4Mask
		) {
			int workspace = (3 + active_workspace()) % 4;
			if (event.xkey.state & ShiftMask)
				set_workspace(active_window(), workspace);
			activate_workspace(workspace);
		} else if (event.xkey.keycode == keycodes[KcTab]) {
			if (!window_list)
				window_list_length = visible_windows(&window_list);
			if (event.xkey.state & ShiftMask) page_previous();
			else page_next();
			XGrabKeyboard(display, root.handle, False,
				GrabModeAsync, GrabModeAsync, event.xkey.time);
			XSync(display, False);
		} else if (event.xkey.keycode == keycodes[KcL]) {
			run(LOCKER, LOCKERARGS);
		} else if (event.xkey.keycode == keycodes[KcR]) {
			run(RUNNER, RUNNERARGS);
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
			run(VOLUME_CONTROL, RAISE_VOLUME);
		} else if (event.xkey.keycode == keycodes[KcXF86AudioLowerVolume]) {
			run(VOLUME_CONTROL, LOWER_VOLUME);
		} else if (event.xkey.keycode == keycodes[KcXF86AudioMute]) {
			run(VOLUME_CONTROL, TOGGLE_MUTE);
		} else if (event.xkey.keycode == keycodes[KcUp]) toggle_fullscreen();
		else puts("caught unhandled keystroke, your KEY_EXPANDO list "
				"may be out of sync with keys.c");
		break;
	case KeyRelease:
		if (
			event.xkey.keycode == keycodes[KcControl_L] ||
			event.xkey.keycode == keycodes[KcControl_R]
		) {
			XUngrabKeyboard(display, event.xkey.time);
			if (ControlMask == prtscrn_state) {
				run(PRTSCRN, CONTROL_PRTSCRN_ARGS);
				prtscrn_state = None;
			}
		}
		if (
			event.xkey.keycode == keycodes[KcSuper_L] ||
			event.xkey.keycode == keycodes[KcSuper_R]
		) {
			XUngrabKeyboard(display, event.xkey.time);
			if (Mod4Mask == prtscrn_state) {
				run(PRTSCRN, SUPER_PRTSCRN_ARGS);
				prtscrn_state = None;
			}
		}
		if (
			event.xkey.keycode == keycodes[KcAlt_L] ||
			event.xkey.keycode == keycodes[KcAlt_R]
		) {
			page_end();
			XUngrabKeyboard(display, event.xkey.time);
			// finalize window restack
		}
		fflush(stdout);
		break;
	}
}

static void numlock_ignoring_hook (int kc, unsigned mod) {
	XGrabKey(display, kc, Mod2Mask | mod, root.handle, True,
		GrabModeAsync, GrabModeAsync);
	XGrabKey(display, kc,            mod, root.handle, True,
		GrabModeAsync, GrabModeAsync);
}

void unhook_keys(void) {
#define KEY_EXPANDO(x) XUngrabKey(display, keycodes[Kc ## x], AnyModifier, root.handle);
#include "expandos.h"
}

void hook_keys (void) {
#define KEY_EXPANDO(x) keycodes[Kc ## x] = XKeysymToKeycode(display, XStringToKeysym(keycode_names[Kc ## x]));
#include "expandos.h"

	numlock_ignoring_hook(keycodes[KcXF86AudioRaiseVolume], None);
	numlock_ignoring_hook(keycodes[KcXF86AudioLowerVolume], None);
	numlock_ignoring_hook(keycodes[KcXF86AudioMute], None);
	numlock_ignoring_hook(keycodes[KcPrint], ControlMask);
	numlock_ignoring_hook(keycodes[KcPrint], Mod4Mask);
	numlock_ignoring_hook(keycodes[KcPrint], None);
	numlock_ignoring_hook(keycodes[KcRight], Mod4Mask | ShiftMask);
	numlock_ignoring_hook(keycodes[KcLeft], Mod4Mask | ShiftMask);
	numlock_ignoring_hook(keycodes[KcRight], Mod4Mask);
	numlock_ignoring_hook(keycodes[KcLeft], Mod4Mask);
	numlock_ignoring_hook(keycodes[KcUp], Mod4Mask);
	numlock_ignoring_hook(keycodes[KcTab], Mod1Mask | ShiftMask);
	numlock_ignoring_hook(keycodes[KcTab], Mod1Mask);
	numlock_ignoring_hook(keycodes[KcF4], Mod1Mask);
	numlock_ignoring_hook(keycodes[KcL], Mod4Mask);
	numlock_ignoring_hook(keycodes[KcR], Mod4Mask);
	numlock_ignoring_hook(keycodes[Kc1], Mod4Mask | ShiftMask);
	numlock_ignoring_hook(keycodes[Kc2], Mod4Mask | ShiftMask);
	numlock_ignoring_hook(keycodes[Kc3], Mod4Mask | ShiftMask);
	numlock_ignoring_hook(keycodes[Kc4], Mod4Mask | ShiftMask);
	numlock_ignoring_hook(keycodes[Kc1], Mod4Mask);
	numlock_ignoring_hook(keycodes[Kc2], Mod4Mask);
	numlock_ignoring_hook(keycodes[Kc3], Mod4Mask);
	numlock_ignoring_hook(keycodes[Kc4], Mod4Mask);
}
