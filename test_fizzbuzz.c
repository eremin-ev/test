/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 */

#include <stdio.h>

static int is_multiple_of(int n, int d)
{
	return n && n % d == 0;
}

int main(void)
{
	int i;
	int N = 101;

	for (i = 0; i < N; i++) {
		int done = 0;

		if (is_multiple_of(i, 3)) {
			printf("fizz");
			done = 1;
		}

		if (is_multiple_of(i, 5)) {
			printf("buzz");
			done = 1;
		}

		if (!done) {
			printf("%d", i);
		}

		printf(" ");
	}

	printf("\n");

	return 0;
}
