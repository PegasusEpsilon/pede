char *event_names[] = {
#define MAGIC_EXPANDO(x) [x] = #x,
#include "eventlist.h"
};
