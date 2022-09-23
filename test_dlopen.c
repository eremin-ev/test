/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 */

#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>

int main(void)
{
	const char *path = "libpython3.8.so.1.0";
	void *h = dlopen(path, RTLD_NOW);
	if (!h) {
		printf("Cannot open '%s'\n", path);
		return EXIT_FAILURE;
	}

	printf("Ok '%s': %p\n", path, h);

	dlclose(h);

	return EXIT_SUCCESS;
}
