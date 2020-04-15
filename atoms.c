/* atoms.c from Pegasus Epsilon's Desktop Environment
** Goal: Less than a megabyte resident
** (C)opyright 2019-2020 "Pegasus Epsilon" <pegasus@pimpninjas.org>
** Distribute Unmodified - https://pegasus.pimpninjas.org/license
*/

#include <X11/Xlib.h>	// XInternAtom()
#include <string.h> 	// strncmp(), strchr(), strncpy(), strncat()
#include <stdlib.h> 	// calloc(), exit(), free()

enum Atoms {
	WM_Sn,
#define ATOM_EXPANDO(x) x,
#include "expandos.h"
	ATOMS_LENGTH
};
Atom atom[ATOMS_LENGTH];

static char *atom_names[] = {
	[WM_Sn] = NULL,
// Not needed, yet...
//#define ATOM_EXPANDO(x) [x] = #x,
//#include "expandos.h"
};

void shutdown_atom_cache (void) { free(atom_names[WM_Sn]); }

void initialize_atom_cache (Display *display, char **envp) {
	// parse DISPLAY environment variable, and construct WM_S<screen number>
	// atom name per ICCCM § Discriminated Names and § Manager Selections
	for (unsigned i = 0; envp[i]; i++)
		if (0 == strncmp(envp[i], "DISPLAY=:", 9)) {
			char *end, *start = envp[i] + 9;
			// This looks unsafe, but validity of the DISPLAY environment
			// variable is already assured by XOpenDisplay before we ever
			// get here. Maybe we won't understand the format of it, but
			// we know it's *valid*, at least, so we can suck some sort of
			// data out of it. Even if it's just a null string, which will
			// result in WM_S. That's good enough, so long as we don't
			// have to deal with another window manager. If the format of
			// the DISPLAY environment variable ever changes, though, we
			// *should* update this for the sake of continuing to operate
			// according to the relevant standards, but we will at least
			// continue to function (wrongly) without an update.
			if (NULL == (end = strchr(start, '.'))) end = strchr(start, 0);
			size_t len = end - start;
			atom_names[WM_Sn] = calloc(1, len + 5);
			strncat(strncpy(atom_names[WM_Sn], "WM_S", 5), start, len);
			break;
		}
	atom[WM_Sn] = XInternAtom(display, atom_names[WM_Sn], False);

#define ATOM_EXPANDO(x) atom[x] = XInternAtom(display, #x, False);
#include "expandos.h"
}
