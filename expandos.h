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
ATOM_EXPANDO(WM_NORMAL_HINTS)
ATOM_EXPANDO(WM_DELETE_WINDOW)
ATOM_EXPANDO(_NET_WM_STATE)
ATOM_EXPANDO(_NET_WM_DESKTOP)	// always on bottom, never focused
ATOM_EXPANDO(_NET_CLIENT_LIST)
ATOM_EXPANDO(_NET_ACTIVE_WINDOW)
ATOM_EXPANDO(_NET_WM_WINDOW_TYPE)
ATOM_EXPANDO(_NET_WM_STATE_STICKY)
ATOM_EXPANDO(_NET_WM_STATE_HIDDEN)
ATOM_EXPANDO(_NET_CURRENT_DESKTOP)
ATOM_EXPANDO(_NET_NUMBER_OF_DISPLAYS)
ATOM_EXPANDO(_NET_WM_STATE_FULLSCREEN)
ATOM_EXPANDO(_NET_SUPPORTING_WM_CHECK)
ATOM_EXPANDO(_NET_WM_WINDOW_TYPE_DESKTOP)
ATOM_EXPANDO(_NET_WM_WINDOW_TYPE_NOTIFICATION)	// always on top, never focused
#undef ATOM_EXPANDO
#endif // ATOM_EXPANDO

#ifdef SIGNAL_EXPANDO
SIGNAL_EXPANDO(HUP) 	// 1
SIGNAL_EXPANDO(INT) 	// 2
SIGNAL_EXPANDO(USR1)	// 10
SIGNAL_EXPANDO(TERM)	// 15
SIGNAL_EXPANDO(CHLD)	// 17
#undef SIGNAL_EXPANDO
#endif // SIGNAL_EXPANDO

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
#endif // EVENT_EXPANDO

#ifdef KEY_EXPANDO
KEY_EXPANDO(XF86AudioRaiseVolume)
KEY_EXPANDO(XF86AudioLowerVolume)
KEY_EXPANDO(XF86AudioMute)
KEY_EXPANDO(Super_L)
KEY_EXPANDO(Super_R)
KEY_EXPANDO(Control_L)
KEY_EXPANDO(Control_R)
KEY_EXPANDO(Alt_L)
KEY_EXPANDO(Alt_R)
KEY_EXPANDO(Print)
KEY_EXPANDO(Right)
KEY_EXPANDO(Left)
KEY_EXPANDO(Up)
KEY_EXPANDO(Tab)
KEY_EXPANDO(F4)
KEY_EXPANDO(L)
KEY_EXPANDO(R)
KEY_EXPANDO(1)
KEY_EXPANDO(2)
KEY_EXPANDO(3)
KEY_EXPANDO(4)
#undef KEY_EXPANDO
#endif // KEY_EXPANDO
