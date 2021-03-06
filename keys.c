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

#include "types.h"
#include "config.h"
#include "wm_core.h"
#include "pager.h"

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

// macro magic because making an inline function
// and using varargs is just far too painful.
#define run(x, ...)                      \
if (!vfork()) {                          \
        setsid();                        \
        execlp(x, x, __VA_ARGS__, NULL); \
}

static unsigned int prtscrn_state = None;
void handle_key_events (XEvent event) {
	switch(event.type) {
	case KeyPress:
		// stupid C rules about switch blocks requiring integer constants...
		if (
			event.xkey.keycode == keycodes[KcRight] &&
			event.xkey.state & Mod4Mask
		) {
			Workspace workspace = (active_workspace() + 1) % 4;
			if (event.xkey.state & ShiftMask) {
				set_workspace(active_window(), workspace);
				XSync(display, True);
			}
			activate_workspace(workspace);
		} else if (
			event.xkey.keycode == keycodes[KcLeft] &&
			event.xkey.state & Mod4Mask
		) {
			Workspace workspace = (active_workspace() + 3) % 4;
			if (event.xkey.state & ShiftMask) {
				set_workspace(active_window(), workspace);
				XSync(display, True);
			}
			activate_workspace(workspace);
		} else if (event.xkey.keycode == keycodes[KcPrint]) {
			event.xkey.state &= ~(unsigned int)Mod2Mask; // ignore numlock bit
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
			event.xkey.keycode == keycodes[KcShift_L] ||
			event.xkey.keycode == keycodes[KcShift_R]
		) { // ignored
		} else if (event.xkey.keycode == keycodes[KcTab]) {
			// ShiftMask == LowerHighest
			page_windows(event.xkey.state & ShiftMask);
			XGrabKeyboard(display, root.handle, False,
				GrabModeAsync, GrabModeAsync, event.xkey.time);
			XSync(display, False);
		} else if (event.xkey.keycode == keycodes[KcL]) {
			run(LOCKER, LOCKERARGS);
#if defined(RUNNER) && defined(RUNNERARGS)
		} else if (event.xkey.keycode == keycodes[KcR]) {
			run(RUNNER, RUNNERARGS);
#endif
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
		} else if (event.xkey.keycode == keycodes[KcUp])
			toggle_fullscreen();
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
			page_windows_end();
			XUngrabKeyboard(display, event.xkey.time);
			// finalize window restack
		}
		fflush(stdout);
		break;
	}
}

static void lock_ignoring_key_hook (int kc, unsigned mod) {
	XGrabKey(display, kc, LockMask | Mod2Mask | mod, root.handle, True,
		GrabModeAsync, GrabModeAsync);
	XGrabKey(display, kc, LockMask |            mod, root.handle, True,
		GrabModeAsync, GrabModeAsync);
	XGrabKey(display, kc,            Mod2Mask | mod, root.handle, True,
		GrabModeAsync, GrabModeAsync);
	XGrabKey(display, kc,                       mod, root.handle, True,
		GrabModeAsync, GrabModeAsync);
}

void unhook_keys(void) {
#define KEY_EXPANDO(x) XUngrabKey(display, keycodes[Kc ## x], AnyModifier, root.handle);
#include "expandos.h"
}

void hook_keys (void) {
#define KEY_EXPANDO(x) keycodes[Kc ## x] = XKeysymToKeycode(display, XStringToKeysym(keycode_names[Kc ## x]));
#include "expandos.h"

	lock_ignoring_key_hook(keycodes[KcXF86AudioRaiseVolume], None);
	lock_ignoring_key_hook(keycodes[KcXF86AudioLowerVolume], None);
	lock_ignoring_key_hook(keycodes[KcXF86AudioMute], None);
	lock_ignoring_key_hook(keycodes[KcPrint], ControlMask);
	lock_ignoring_key_hook(keycodes[KcPrint], Mod4Mask);
	lock_ignoring_key_hook(keycodes[KcPrint], None);
	lock_ignoring_key_hook(keycodes[KcRight], Mod4Mask | ShiftMask);
	lock_ignoring_key_hook(keycodes[KcLeft], Mod4Mask | ShiftMask);
	lock_ignoring_key_hook(keycodes[KcRight], Mod4Mask);
	lock_ignoring_key_hook(keycodes[KcLeft], Mod4Mask);
	lock_ignoring_key_hook(keycodes[KcUp], Mod1Mask | Mod4Mask);
	lock_ignoring_key_hook(keycodes[KcUp], Mod4Mask);
	lock_ignoring_key_hook(keycodes[KcTab], Mod1Mask | ShiftMask);
	lock_ignoring_key_hook(keycodes[KcTab], Mod1Mask);
	lock_ignoring_key_hook(keycodes[KcF4], Mod1Mask);
	lock_ignoring_key_hook(keycodes[KcL], Mod4Mask);
#if defined(RUNNER) && defined(RUNNERARGS)
	lock_ignoring_key_hook(keycodes[KcR], Mod4Mask);
#endif
	lock_ignoring_key_hook(keycodes[Kc1], Mod4Mask | ShiftMask);
	lock_ignoring_key_hook(keycodes[Kc2], Mod4Mask | ShiftMask);
	lock_ignoring_key_hook(keycodes[Kc3], Mod4Mask | ShiftMask);
	lock_ignoring_key_hook(keycodes[Kc4], Mod4Mask | ShiftMask);
	lock_ignoring_key_hook(keycodes[Kc1], Mod4Mask);
	lock_ignoring_key_hook(keycodes[Kc2], Mod4Mask);
	lock_ignoring_key_hook(keycodes[Kc3], Mod4Mask);
	lock_ignoring_key_hook(keycodes[Kc4], Mod4Mask);
}
