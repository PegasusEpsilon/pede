/* wm_core.h - Pegasus Epsilon's Desktop Environment
** Goal: Less than a megabyte resident
** (C)opyright 2019-2020 "Pegasus Epsilon" <pegasus@pimpninjas.org>
** Distribute Unmodified - https://pegasus.pimpninjas.org/license
*/

#ifndef WM_H
#define WM_H

#include <X11/Xlib.h>
#include <stdint.h>

#include "types.h"

extern long long nul;
#define VOID ((void *)&nul)
extern Display *display;
extern struct { Window handle; unsigned width, height; } root;
extern Window pede;
extern uint32_t active_workspace (void);
Window active_window (void);
void focus_window (Window);
void focus_active_window (void);
void activate_workspace (const Workspace);
void set_workspace (const Window, const Workspace);
char *window_title (Window);
void set_sticky (Window);
/* TODO:
void clear_sticky (Window);
void set_bottom (Window);
void set_top (Window);
*/
void *XGetWindowPropertyArray (Window, Atom, Atom, long unsigned *);
Bool XWindowPropertyArrayContains (Window, Atom, Atom);
unsigned visible_windows (Window **);
void update_client_list (void);
void window_diagnostic (char *, Window, char *);
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
