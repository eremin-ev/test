/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 */

#include <stdio.h>
#include <inttypes.h>

int main(void)
{
	uint64_t start = 0x123456789L;
	uint64_t size = 0x987654321;

	char *addr1 = (char *)(intptr_t)start;
	char *addr2 = (char *)(intptr_t)size;

	printf("start 0x%" PRIx64 ", size 0x%" PRIx64 "\n", start, size);
	printf("addr1 %p, addr2 %p\n", addr1, addr2);

	unsigned u = -1;

	printf("%x\n", u);

	return 0;
}
