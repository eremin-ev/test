/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 */

#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>

bool test_dlopen(const char *path)
{
    void *h = dlopen(path, RTLD_NOW);
    if (!h) {
        printf("Cannot open '%s': %s\n", path, dlerror());
        return false;
    }

    printf("Ok '%s': %p\n", path, h);
    dlclose(h);

    return true;
}

int main(int argc, const char **argv)
{
    for (int r = 1; r < argc; ++r) {
        printf("loading '%s'...\n", argv[r]);
        if (!test_dlopen(argv[r])) {
            return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
}
