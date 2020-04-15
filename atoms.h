/* atoms.h from Pegasus Epsilon's Desktop Environment
** Goal: Less than a megabyte resident
** (C)opyright 2019-2020 "Pegasus Epsilon" <pegasus@pimpninjas.org>
** Distribute Unmodified - https://pegasus.pimpninjas.org/license
*/

#ifndef ATOMS_H
#define ATOMS_H
enum Atoms {
	WM_Sn,
#define ATOM_EXPANDO(x) x,
#include "expandos.h"
	ATOMS_LENGTH
};
extern Atom atom[ATOMS_LENGTH];

void shutdown_atom_cache (void);
void initialize_atom_cache (Display *display, char **envp);
#endif /* ATOMS_H */
