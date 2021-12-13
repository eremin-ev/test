/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void test_str_diff(const char **lines0, const char **lines1)
{
    const char **p0 = lines0;
    const char **p1 = lines1;
    int r;

    while (1) {
        if (*p0 && *p1) {
            r = strcmp(*p0, *p1);
            if (r != 0) {
                printf("removed '%s'\n", *p0);
                printf("added '%s'\n", *p1);
            //} else {
            //    printf("no changes in '%s' and '%s'\n", *p0, *p1);
            }
            ++p0;
            ++p1;
        } else if (!*p0 && *p1) {
            printf("added '%s'\n", *p1);
            ++p1;
        } else if (*p0 && !*p1) {
            printf("removed '%s'\n", *p0);
            ++p0;
        } else {
            //printf("nothing left to proceed with, stop\n");
            break;
        }
    }

    printf("---\n");
}

int main()
{
    const char *l0[] = {
        "0: init",
        NULL,
    };

    const char *l1[] = {
        "0: init",
        "1: first",
        NULL,
    };

    const char *l2[] = {
        "0: init",
        "1: first",
        "2: second",
        NULL,
    };

    const char *l2o[] = {
        "0: init",
        "1: other first",
        "2: other second",
        NULL,
    };

    const char *l2m[] = {
        "0: init",
        "1: second",
        "2: first",
        NULL,
    };

    test_str_diff(l0, l1);
    test_str_diff(l0, l2);
    test_str_diff(l1, l0);
    test_str_diff(l2, l0);
    test_str_diff(l2, l2o);
    test_str_diff(l2, l2m);

    return EXIT_SUCCESS;
}
