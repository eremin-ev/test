/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 */

#include <stdio.h>
#include <stdlib.h>

#include <systemd/sd-login.h>

void test_get_seats(void)
{
	char **seats;
	int nr = sd_get_seats(&seats);
	int i;
	for (i = 0; i < nr; i++) {
		printf("%s %d %s\n", __func__, i, seats[i]);
		free(seats[i]);
	}
	free(seats);
}

int main(void)
{
	test_get_seats();

	return 0;
}
