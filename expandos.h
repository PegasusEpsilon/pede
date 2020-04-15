/* expandos.h from Pegasus Epsilon's Desktop Environment
** Goal: Less than a megabyte resident
** (C)opyright 2019-2020 "Pegasus Epsilon" <pegasus@pimpninjas.org>
** Distribute Unmodified - https://pegasus.pimpninjas.org/license
*/

#ifdef ATOM_EXPANDO
ATOM_EXPANDO(ATOM)
ATOM_EXPANDO(WINDOW)
ATOM_EXPANDO(CARDINAL)
ATOM_EXPANDO(UTF8_STRING)
ATOM_EXPANDO(WM_PROTOCOLS)
ATOM_EXPANDO(WM_DELETE_WINDOW)
ATOM_EXPANDO(_NET_WM_STATE)
ATOM_EXPANDO(_NET_WM_DESKTOP)
ATOM_EXPANDO(_NET_ACTIVE_WINDOW)
ATOM_EXPANDO(_NET_CURRENT_DESKTOP)
ATOM_EXPANDO(_NET_NUMBER_OF_DISPLAYS)
ATOM_EXPANDO(_NET_SUPPORTING_WM_CHECK)
#undef ATOM_EXPANDO
#endif

#ifdef SIGNAL_EXPANDO
SIGNAL_EXPANDO(INT)
SIGNAL_EXPANDO(TERM)
SIGNAL_EXPANDO(USR1)
#undef SIGNAL_EXPANDO
#endif

#ifdef EVENT_EXPANDO
EVENT_EXPANDO(MapNotify)
EVENT_EXPANDO(ConfigureRequest)
EVENT_EXPANDO(Expose)
EVENT_EXPANDO(SelectionRequest)
EVENT_EXPANDO(ClientMessage)
EVENT_EXPANDO(MapRequest)
EVENT_EXPANDO(CirculateNotify)
EVENT_EXPANDO(SelectionClear)
EVENT_EXPANDO(KeyPress)
EVENT_EXPANDO(ButtonPress)
EVENT_EXPANDO(MotionNotify)
EVENT_EXPANDO(ButtonRelease)
EVENT_EXPANDO(CreateNotify)
EVENT_EXPANDO(MappingNotify)
EVENT_EXPANDO(UnmapNotify)
EVENT_EXPANDO(DestroyNotify)
EVENT_EXPANDO(KeyRelease)
EVENT_EXPANDO(ConfigureNotify)
EVENT_EXPANDO(PropertyNotify)
#undef EVENT_EXPANDO
#endif