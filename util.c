#include <stdint.h>
#include <stdio.h>

unsigned long delete_int64_t_from_array (
	int64_t *array, unsigned long register length, int64_t element
) {
	unsigned long register src = 0, dst = 0;
	while (src < length) {
		while (array[src] == element)
			if (++src >= length) return dst;
		if (src != dst) array[dst] = array[src];
		src++; dst++;
	}
	return dst;
}
