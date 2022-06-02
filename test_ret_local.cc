/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 */

#include <iostream>

#include <stdio.h>

char *foo(int a)
{
    char buf[16];
    snprintf(buf, 16, "%d", a);
    return buf;
}

int main(void)
{
    int a = rand();
    int b = rand();

    char *s1 = foo(a);
    char *s2 = foo(b);

    printf("s1 %s, s2 %s\n", s1, s2);

    int r;
    int *rr = &r;

    r = 0;

    if (*rr) {
        std::cout << "*rr is true: " << (*rr) << '\n';
    }

    if (*rr == 0) {
        std::cout << "*rr is 0: " << (*rr) << '\n';
    }

    return 0;
}
