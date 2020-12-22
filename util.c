#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

size_t XDeleteLongFromArray (
	long unsigned *array, const size_t length, bool (*delete)(long unsigned)
) {
	size_t src = 0, dst = 0;
	while (length > src) {
		while (delete(array[src]) && length > ++src);
		if (length <= src) break;
		if (src != dst) array[dst] = array[src];
		src++; dst++;
	}
	return dst;
}
