/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 */

#include <stdio.h>
#include <string.h>

void *my_memmove(void *dest, const void *src, size_t n)
{
	char *s = (char *)src;
	char *d = (char *)dest;
	int idx;
	if (dest > src) {
		printf("%s dest > src\n", __func__);
		for (size_t i=0; i<n; i++) {
			idx = n - i - 1;
			d[idx] = s[idx];
		}
	} else {
		for (size_t i=0; i<n; i++) {
			d[i] = s[i];
		}
	}
}

int main(void)
{
	char a[20] = {  'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'q', 'i',
			'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', };
	char *b = a + 1;
	int i;

	for (i = 0; i < 10; i++) {
		printf("%c ", a[i]);
	}
	printf("\n");

	//my_memmove(b, a, 10);
	memcpy(b, a, 10);

	for (i = 0; i < 10; i++) {
		printf("%c ", b[i]);
	}
	printf("\n");

	for (i = 0; i < 10; i++) {
		printf("%c ", a[i]);
	}
	printf("\n");


	return 0;
}
