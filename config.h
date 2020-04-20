/* config.h - Pegasus Epsilon's Desktop Environment
** Goal: Less than a megabyte resident
** (C)opyright 2019-2020 "Pegasus Epsilon" <pegasus@pimpninjas.org>
** Distribute Unmodified - https://pegasus.pimpninjas.org/license
*/

#ifndef CONFIG_H
#define CONFIG_H

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

#define LOCKER "xscreensaver-command"
#define LOCKERARGS "-lock"

// VOLUME_CONTROL should be the name of the program you want to use
// to control your system volume. Good examples are pactl or amixer
#define VOLUME_CONTROL "pactl"

// RAISE_VOLUME, LOWER_VOLUME, and TOGGLE_MUTE should be the required
// arguments to pass to VOLUME_CONTROL that do what they say.
#define RAISE_VOLUME "set-sink-volume", "@DEFAULT_SINK@", "+5%"
#define LOWER_VOLUME "set-sink-volume", "@DEFAULT_SINK@", "-5%"
#define TOGGLE_MUTE "set-sink-mute", "@DEFAULT_SINK@", "toggle"

#define SCROT_ARGS \
	"xclip -selection clipboard -t image/png -i $f;" \
	"mv $f ~/Pictures/scrot"

#define PRTSCRN "scrot"
#define PRTSCRN_ARGS "-e", SCROT_ARGS
#define SUPER_PRTSCRN_ARGS "-ue", SCROT_ARGS
#define CONTROL_PRTSCRN_ARGS "-se", SCROT_ARGS

// minimum window size (height and width) - do not go below 1
#define MINIMUM_SIZE 100
// snap distance, 0 to disable
#define SNAP 25

#endif // CONFIG_H
