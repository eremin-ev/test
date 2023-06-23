/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 *
 * Prefix match with other strings
 *
 * https://practice.geeksforgeeks.org/problems/prefix-match-with-other-strings/1
 */

#include <iostream>
#include <string>

#include <string.h>

#define ARRAY_SIZE(a)	(sizeof(a) / sizeof(a[0]))

static int klengthpref(const std::string arr[], int n, int k, std::string str)
{
    int nr = 0;
    long unsigned l = static_cast<long unsigned>(k);

    if (str.size() < l) {
        return 0;
    }

    for (int i = 0; i < n; ++i) {
        if (l <= arr[i].size() && 0 == strncmp(arr[i].c_str(), str.c_str(), k)) {
            ++nr;
        }
        std::cout << __func__ << " " << arr[i] << '\n';
    }

    return nr;
}

int main()
{
    const std::string arr1[] = { "abba", "abbb", "abbc", "abbd", "abaa", "abca" };
    const std::string arr2[] = { "geeks", "geeksforgeeks", "forgeeks" };
    const std::string arr3[] = { "xc", "sce", "lyhspnz" };

    struct {
        const std::string *arr;
        int arr_size;
        const std::string str;
        int k;
        int exp;
    } cases[] = {
        {
            .arr = arr1,
            .arr_size = ARRAY_SIZE(arr1),
            .str = "abbg",
            .k = 3,
            .exp = 4,
        },
        {
            .arr = arr2,
            .arr_size = ARRAY_SIZE(arr2),
            .str = "geeks",
            .k = 2,
            .exp = 2,
        },
        {
            .arr = arr3,
            .arr_size = ARRAY_SIZE(arr3),
            .str = "xc",
            .k = 10,
            .exp = 0,
        },
    };

    for (const auto &c : cases) {
        int r = klengthpref(c.arr, c.arr_size, c.k, c.str);
        std::cout << c.str << " -> "
                  << r << " <?> " << c.exp << " : "
                  << (r == c.exp ? "ok" : "failed" ) << '\n';
    }

    return 0;
}
