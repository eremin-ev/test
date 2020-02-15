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
	int i = 314, j = 271;
	int *pi = &i, *pj = &j;
	int &ri = i, &rj = j;

	printf("%i %i\n", ri, rj);
	printf("%i %i\n", *pi, *pj);

	return 0;
}
