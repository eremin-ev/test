/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 */

#include <stdio.h>
#include <string.h>

#define PAGE_SIZE 4096

int main(void)
{
	char a[PAGE_SIZE] = { [0 ... PAGE_SIZE - 1] = 0x73 };
	unsigned int *i, *i0;

	i = (void *)a;

	for (i0 = i; i - i0 < PAGE_SIZE/sizeof(unsigned int); ++i) {
		printf("%lx %lx %08x\n", i - i0, (void *)i - (void *)i0, *i);
	}
	printf("\n");

	return 0;
}
