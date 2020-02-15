/*
 * A simple reference vs pointer test.
 *
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 */

#include <stdio.h>
#include <math.h>


int main()
{
	int i = 314;
	int *pi = &i;
	int &ri = i;

	printf("%i %i %i\n", i, ri, *pi);

	ri = 271;

	printf("%i %i %i\n", i, ri, *pi);

	*pi = 161;

	printf("%i %i %i\n", i, ri, *pi);

	return 0;
}
