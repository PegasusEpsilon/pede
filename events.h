char *event_names[] = {
#define EVENT_EXPANDO(x) [x] = #x,
#include "expandos.h"
};
