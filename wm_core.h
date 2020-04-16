/* wm.h - Pegasus Epsilon's Desktop Environment
** Goal: Less than a megabyte resident
** (C)opyright 2019-2020 "Pegasus Epsilon" <pegasus@pimpninjas.org>
** Distribute Unmodified - https://pegasus.pimpninjas.org/license
*/

#ifndef WM_H
#define WM_H

extern long long nul;
#define VOID ((void *)&nul)
#define BUFFER_LENGTH 512
extern char buffer[BUFFER_LENGTH];
extern Display *display;
extern struct { Window handle; unsigned int width, height; } root;
extern Window pede;
extern unsigned char active_workspace (Display *);
Window active_window (Display *);
void focus_window (Display *, Window);
void focus_active_window (Display *);
void activate_workspace (Display *, const uint32_t);
void set_workspace (Display *, Window, uint32_t);
unsigned long XDeleteAtomFromArray (Atom *, unsigned long, Atom);
void *XGetWindowPropertyArray (Display *, Window, Atom, Atom, unsigned long *);
Bool XWindowPropertyArrayContains (Display *, Window, Atom, Atom);
void close_window (Display *, Window);
void remove_state (Display *, Window, Atom);
void add_state (Display *, Window, Atom);
void alter_window_state (XClientMessageEvent);
void maximize_window (Display *, Window);
void refuse_selection_request (XSelectionRequestEvent);
void become_wm (Display *, Window);
void map_window (XMapRequestEvent *);

#endif // WM_H
