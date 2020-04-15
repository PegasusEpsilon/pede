/* signallist.h from Pegasus Epsilon's Desktop Environment
** Goal: Less than a megabyte resident
** (C)opyright 2019-2020 "Pegasus Epsilon" <pegasus@pimpninjas.org>
** Distribute Unmodified - https://pegasus.pimpninjas.org/license
*/
#ifdef MAGIC_EXPANDO
MAGIC_EXPANDO(INT)
MAGIC_EXPANDO(TERM)
MAGIC_EXPANDO(USR1)
#undef MAGIC_EXPANDO
#else
#error Inclusion of signallist.h without defining MAGIC_EXPANDO does nothing
#endif
