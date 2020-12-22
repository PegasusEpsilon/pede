/* config.h - Pegasus Epsilon's Desktop Environment
** Goal: Less than a megabyte resident
** (C)opyright 2019-2020 "Pegasus Epsilon" <pegasus@pimpninjas.org>
** Distribute Unmodified - https://pegasus.pimpninjas.org/license
*/

#ifndef CONFIG_H
#define CONFIG_H

// comma-separated list of window move/resize modifiers.
// they pretty much do what they say on the tin.
// remove whichever modifiers you don't like to disable only that one.
//
// valid MOVE_MODIFIERS:
//   keep_on_screen - keeps all parts of all windows on the visible screen
//   snap_to_edges - snaps windows to screen edges
//   snap_to_siblings - snaps windows to each other
//   snap_to_center - snaps windows to center of screen (vertical and horizontal)
//
// valid RESIZE_MODIFIERS:
//   keep_on_screen - same as above, but when resizing
//   snap_to_edges - same as above, but when resizing
//   snap_to_siblings - same as above, but when resizing
//   snap_to_center - same as above, but when resizing
//
#define MOVE_MODIFIERS keep_on_screen, snap_to_edges, snap_to_siblings, snap_to_center
#define RESIZE_MODIFIERS keep_on_screen, snap_to_edges, snap_to_siblings, snap_to_center

// snap distance, 0 to disable ALL snapping (but not keep_on_screen!)
#define SNAP 25

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

// We do not (yet) have support for "dm-tool lock" and other such shenanigans
// as that requires cooperation from the window manager...
#define LOCKER "xscreensaver-command"
#define LOCKERARGS "-lock"

// VOLUME_CONTROL should be the name of the program you want to use
// to control your system volume. Good examples are pactl or amixer
#define VOLUME_CONTROL "/home/pegasus/.pede/pulse-volume"

// RAISE_VOLUME, LOWER_VOLUME, and TOGGLE_MUTE should be the required
// arguments to pass to VOLUME_CONTROL that do what they say.
#define RAISE_VOLUME "set-sink-volume", "+5%"
#define LOWER_VOLUME "set-sink-volume", "-5%"
#define TOGGLE_MUTE "set-sink-mute", "toggle"

#define SCROT_ARGS \
	"xclip -selection clipboard -t image/png -i $f;" \
	"mv $f ~/Pictures/scrot"

#define PRTSCRN "scrot"
#define PRTSCRN_ARGS "-e", SCROT_ARGS
#define SUPER_PRTSCRN_ARGS "-ue", SCROT_ARGS
#define CONTROL_PRTSCRN_ARGS "-se", SCROT_ARGS

// minimum window size (height and width) - do not go below 1
#define MINIMUM_SIZE 100

#define ALT_FULLSCREEN_WIDTH 1920
#define ALT_FULLSCREEN_HEIGHT 1080

#endif // CONFIG_H
