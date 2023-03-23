/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 */

/*
 * From yandex interview
 */

#include <algorithm>

#include <limits.h>
#include <stdio.h>

int dist_v1(const char *s)
{
    int min1 = INT_MAX;
    int expect = 0;
    int distance = 0;
    while (*s) {
        switch (*s) {
        case 'X':
            if (expect == 'X') {
                min1 = std::min(min1, distance);
            }
            expect = 'Y';
            distance = 0;
            break;
        case 'Y':
            if (expect == 'Y') {
                min1 = std::min(min1, distance);
            }
            expect = 'X';
            distance = 0;
            break;
        case 'O':
            break;
        default:
            return 0;
        }

        if (expect) {
            distance++;
        }

        s++;
    }

    if (min1 == INT_MAX) {
        return 0;
    }

    return min1;
}

int dist_v2(const char *s)
{
    int min1 = INT_MAX;
    const char *start = nullptr;
    while (*s) {
        switch (*s) {
        case 'X':
        case 'Y':
            if (start && *start != *s) {
                min1 = std::min(min1, static_cast<int>(s - start));
            }
            start = s;
            break;
        case 'O':
            break;
        default:
            return 0;
        }

        s++;
    }

    if (min1 == INT_MAX) {
        return 0;
    }

    return min1;
}

void test_dist()
{
    struct X {
        const char *s;
        int d;
    };

    const struct X x[] = {
        { .s = "XX", .d = 0 },
        { .s = "YY", .d = 0 },
        { .s = "XY", .d = 1 },
        { .s = "YX", .d = 1 },
        { .s = "OOYOY", .d = 0 },
        { .s = "OOYOXOO", .d = 2 },
        { .s = "OOYOXOXYO", .d = 1 },
        { .s = "OOYOXOOOOXYO", .d = 1 },
        { .s = "OOYOXOOOXOOOXYX", .d = 1 },
        { .s = "OOYOXOXYOx", .d = 0 },
    };

    for (const auto &i : x) {
        int d1 = dist_v1(i.s);
        int d2 = dist_v2(i.s);
        printf("%s %s: %i %s %i %s\n", __func__, i.s,
               d1, d1 == i.d ? "ok" : "failed",
               d2, d2 == i.d ? "ok" : "failed");
    }
}

int main()
{
    test_dist();

    return 0;
}
