#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

bool long_array_contains (
	long unsigned needle, long unsigned *haystack, long unsigned count
) {
	while (haystack && count--)
		if (needle == haystack[count])
			return true;
	return false;
}

size_t delete_long_from_array (
	long unsigned *array, const size_t length, bool (*keep)(long unsigned)
) {
	size_t src = 0, dst = 0;
	while (length > src) {
		while (!keep(array[src]) && length > ++src);
		if (length <= src) break;
		if (src != dst) array[dst] = array[src];
		src++; dst++;
	}
	return dst;
}
