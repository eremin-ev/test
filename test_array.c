/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 */

#include <stdio.h>

int main(void)
{
	int a[] = {
		1, 2, 3, 4, 5, 6,
	};
	void *ap1 = a;
	void *ap2 = &a;
	void *ap3 = &a[0];

	printf("a %p, &a %p, &a[0] %p, a == &a '%s', &a == &a[0] '%s'\n",
		ap1, ap2, ap3,
		ap1 == ap2 ? "OK" : "--",
		ap2 == ap3 ? "OK" : "--");

	return 0;
}
