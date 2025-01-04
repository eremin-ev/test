/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 *
 * Stolen from Linux Kernel [1].
 *
 * [1] https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/tree/lib/math/gcd.c?h=master
 */

#include <utility>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

static unsigned long gcd(unsigned long a, unsigned long b)
{
    unsigned long r = a | b;

    if (!a || !b)
        return r;

    /* Isolate lsbit of r */
    r &= -r;

    while (!(b & r))
        b >>= 1;

    if (b == r)
        return r;

    for (;;) {
        while (!(a & r))
            a >>= 1;

        if (a == r)
            return r;

        if (a == b)
            return a;

        if (a < b)
            std::swap(a, b);

        a -= b;
        a >>= 1;

        if (a & r)
            a += b;

        a >>= 1;
    }
}

int main(int argc, char **argv)
{
    if (argc != 3) {
        printf("usage: %s <a> <b>\n", argv[0]);
        return EINVAL;
    }

    unsigned long a = strtol(argv[1], NULL, 10);
    unsigned long b = strtol(argv[2], NULL, 10);

    printf("gcd(%lu, %lu) %lu\n", a, b, gcd(a, b));

    return 0;
}
