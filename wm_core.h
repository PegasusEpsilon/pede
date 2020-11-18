/* wm_core.h - Pegasus Epsilon's Desktop Environment
** Goal: Less than a megabyte resident
** (C)opyright 2019-2020 "Pegasus Epsilon" <pegasus@pimpninjas.org>
** Distribute Unmodified - https://pegasus.pimpninjas.org/license
*/

#ifndef WM_H
#define WM_H

#include <X11/Xlib.h>
#include <stdint.h>

extern long long nul;
#define VOID ((void *)&nul)
extern Display *display;
extern struct { Window handle; unsigned width, height; } root;
extern Window pede;
extern unsigned char active_workspace (void);
Window active_window (void);
void focus_window (Window);
void focus_active_window (void);
void activate_workspace (const uint32_t);
void set_workspace (Window, uint32_t);
char *window_title (Window);
void *XGetWindowPropertyArray (Window, Atom, Atom, unsigned long *);
Bool XWindowPropertyArrayContains (Window, Atom, Atom);
unsigned visible_windows(Window **windows);
void toggle_fullscreen (void);
void close_window (Window);
void remove_state (Window, Atom);
void add_state (Window, Atom);
void alter_window_state (XClientMessageEvent);
void maximize_window (Window);
void refuse_selection_request (XSelectionRequestEvent);
void make_wm (Window);
void map_window (XMapRequestEvent *);

#endif // WM_H
