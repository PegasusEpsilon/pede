#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

static long unsigned target_long = 0;
bool long_compare (long unsigned test) { return target_long != test; }
bool (*long_isnt (long unsigned target)) (long unsigned) {
	target_long = target;
	return &long_compare;
}

void swap_long_array (long unsigned *array, size_t a, size_t b) {
	array[a] ^= array[b];
	array[b] ^= array[a];
	array[a] ^= array[b];
}

void rotate_long_array_up (long unsigned *array, size_t len) {
	for (size_t i = 1; i < len; i++) swap_long_array(array, i, 0);
}

void rotate_long_array_down (long unsigned *array, size_t len) {
	for (size_t i = len; --i;) swap_long_array(array, i, 0);
}

void reverse_long_array (long unsigned *array, size_t len) {
	for (size_t i = len-- / 2; i--;) swap_long_array(array, i, len - i);
}

bool long_array_contains (
	long unsigned needle, long unsigned *haystack, size_t len
) {
	while (haystack && len--)
		if (needle == haystack[len])
			return true;
	return false;
}

size_t delete_long_from_array (
	long unsigned *array, const size_t len, bool (*keep)(long unsigned)
) {
	size_t src = 0, dst = 0;
	while (len > src) {
		while (!keep(array[src]) && len > ++src);
		if (len <= src) break;
		if (src != dst) array[dst] = array[src];
		src++; dst++;
	}
	return dst;
}
