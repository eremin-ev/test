/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 */

#include <stdio.h>

int main(void)
{
	int a = 1;
	int b = 0;
	double c = 1.0;
	double d = 0.0;

	printf("double %g\n", c/d);

	printf("int %i\n", a/b);

	return 0;
}
