/* atomlist.h from Pegasus Epsilon's Desktop Environment
** Goal: Less than a megabyte resident
** (C)opyright 2019-2020 "Pegasus Epsilon" <pegasus@pimpninjas.org>
** Distribute Unmodified - https://pegasus.pimpninjas.org/license
*/

#ifdef MAGIC_EXPANDO
MAGIC_EXPANDO(ATOM)
MAGIC_EXPANDO(WINDOW)
MAGIC_EXPANDO(CARDINAL)
MAGIC_EXPANDO(UTF8_STRING)
MAGIC_EXPANDO(WM_PROTOCOLS)
MAGIC_EXPANDO(WM_DELETE_WINDOW)
MAGIC_EXPANDO(_NET_WM_STATE)
MAGIC_EXPANDO(_NET_WM_DESKTOP)
MAGIC_EXPANDO(_NET_ACTIVE_WINDOW)
MAGIC_EXPANDO(_NET_CURRENT_DESKTOP)
MAGIC_EXPANDO(_NET_NUMBER_OF_DISPLAYS)
MAGIC_EXPANDO(_NET_SUPPORTING_WM_CHECK)
#undef MAGIC_EXPANDO
#else
#error Inclusion of atomlist.h without defining MAGIC_EXPANDO does nothing
#endif
