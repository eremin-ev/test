/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 */

#include <sys/auxv.h>
#include <stdio.h>
#include <time.h>

int main(int, char **)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	unsigned long p = getauxval(AT_SYSINFO_EHDR);
	printf("AT_SYSINFO_EHDR %lu\n", p);
	return ts.tv_sec > 0 ? 0 : 1;
}
