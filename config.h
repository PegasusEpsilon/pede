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

// minimum window size (height and width) - do not go below 1
#define MINIMUM_SIZE 100
// snap distance, 0 to disable
#define SNAP 25

#endif // CONFIG_H
