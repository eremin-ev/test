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
	int i = 317;
	int *pi = &i;
	int &ri = i;
	printf("%i %i %i\n", i, ri, *pi);
	ri = 271;
	printf("%i %i %i\n", i, ri, *pi);
	*pi = 373;
	printf("%i %i %i\n", i, ri, *pi);

	int *pn = new int;
	printf("pn %p\n", pn);
	delete pn;

	int &rn = *new int;
	rn = 173;
	printf("&rn %p, rn %i\n", &rn, rn);
	delete &rn;

	return 0;
}
