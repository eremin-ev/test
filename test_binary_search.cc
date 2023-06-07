/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 *
 * Binary Search in sorted array.  See Binary Search [1]
 *
 * [1] https://practice.geeksforgeeks.org/problems/binary-search-1587115620/1
 */

#include <iostream>

namespace {

int binarysearch(int arr[], int n, int k)
{
        if (!n) {
            return -1;
        }

        if (n == 1) {
            if (arr[0] == k) {
                return 0;
            } else {
                return -1;
            }
        }

        int n1 = n/2 + (n & 1);
        int n2 = n/2;
        int idx = n1 - 1;
        int el = arr[idx];
        if (k < el) {
            idx = binarysearch(arr, n1, k);
            if (idx == -1) {
                return -1;
            }

            return idx;
        } else if (k > el) {
            idx = binarysearch(arr + n1, n2, k);
            if (idx == -1) {
                return -1;
            }

            return n1 + idx;
        }

        return idx;
}

} // anonymous namespace

int main()
{
    int arr1[] = {1, 2, 3, 4, 5};
    int arr2[] = {11, 22, 33, 44, 55};
    struct test_bs {
        int *arr; // array
        int n;    // arrray size
        int k;    // search for element
        int exp;  // expected index
    } cases[] = {
        {
            .arr = arr1,
            .n = 5,
            .k = 4,
            .exp = 3,
        }, {
            .arr = arr2,
            .n = 5,
            .k = 445,
            .exp = -1,
        }
    };

    for (const auto &c : cases) {
        int idx = binarysearch(c.arr, c.n, c.k);
        std::cout << idx << ' ' << (idx == c.exp ? "ok" : "failed") << '\n';
    }

    return 0;
}
