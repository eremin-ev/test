/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 */

#include <stdio.h>

static size_t round_up(size_t v, size_t align)
{
	return (v + align - 1) & ~(align - 1);
}

int main(void)
{
	int i;

	for (i = 0; i < 100; i++) {
		printf("i %i, aligned %zi\n", i, round_up(i, 16));
	}

	return 0;
}
