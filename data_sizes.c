/* data_sizes.c from Pegasus Epsilon's Display Environment
** Goal: Less than a megabyte resident
** (C)opyright 2019-2020 "Pegasus Epsilon" <pegasus@pimpninjas.org>
** Distribute Unmodified - https://pegasus.pimpninjas.org/license
*/

#include <stdio.h>
#include <X11/Xutil.h>

int main (void) {
	puts("#ifndef DATA_SIZES_H");
	puts("#define DATA_SIZES_H");
	printf("#define WINDOW_SIZE %d\n", sizeof(Window));
	printf("#define ATOM_SIZE %d\n", sizeof(Atom));
	puts("#endif // DATA_SIZES_H");
	return 0;
}
