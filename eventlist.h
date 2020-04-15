#ifdef MAGIC_EXPANDO
MAGIC_EXPANDO(MapNotify)
MAGIC_EXPANDO(ConfigureRequest)
MAGIC_EXPANDO(Expose)
MAGIC_EXPANDO(SelectionRequest)
MAGIC_EXPANDO(ClientMessage)
MAGIC_EXPANDO(MapRequest)
MAGIC_EXPANDO(CirculateNotify)
MAGIC_EXPANDO(SelectionClear)
MAGIC_EXPANDO(KeyPress)
MAGIC_EXPANDO(ButtonPress)
MAGIC_EXPANDO(MotionNotify)
MAGIC_EXPANDO(ButtonRelease)
MAGIC_EXPANDO(CreateNotify)
MAGIC_EXPANDO(MappingNotify)
MAGIC_EXPANDO(UnmapNotify)
MAGIC_EXPANDO(DestroyNotify)
MAGIC_EXPANDO(KeyRelease)
MAGIC_EXPANDO(ConfigureNotify)
MAGIC_EXPANDO(PropertyNotify)
#undef MAGIC_EXPANDO
#else
#error Inclusion of eventlist.h without defining MAGIC_EXPANDO does nothing
#endif
