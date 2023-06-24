/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 *
 * Missing number in array [1]
 *
 *      Easy    Accuracy: 29.59%    Submissions: 809K+      Points: 2
 *
 * Given an array of size N-1 such that it only contains distinct
 * integers in the range of 1 to N. Find the missing element.
 *
 * Example 1:
 *
 *  Input:      N = 5, A[] = { 1, 2, 3, 5 }
 *
 *  Output:     4
 *
 * Example 2:
 *
 *  Input:      N = 10, A[] = { 6, 1, 2, 8, 3, 4, 7, 10, 5 }
 *
 *  Output:     9
 *
 * Your Task:
 *
 *
 *      You don't need to read input or print anything. Complete the
 *      function MissingNumber() that takes array and N as input
 *      parameters and returns the value of the missing number.
 *
 * Expected Time Complexity: O(N)
 * Expected Auxiliary Space: O(1)
 *
 * Constraints:
 *
 *      1 <= N <= 10^6
 *      1 <= A_i ≤ 10^6
 *
 *  [1] https://practice.geeksforgeeks.org/problems/missing-number-in-array1416/1
 */

#include <iostream>
#include <vector>

namespace {

static int missingNumber(const std::vector<int> &arr, int n)
{
    long s = 0;
    for (const auto &a : arr) {
        s += a;
    }

    return n*(n + 1)/2 - s;
}

static void test_missing_number()
{
    struct {
        const std::vector<int> arr;
        int exp;
    } cases[] = {
        {
            .arr = { 1, 2, 3, 5 },
            .exp = 4,
        },
        {
            .arr = { 6, 1, 2, 8, 3, 4, 7, 10, 5 },
            .exp = 9,
        },
    };

    for (const auto &c : cases) {
        // (size() + 1) since one number is missing from array
        int N = c.arr.size() + 1;
        int r = missingNumber(c.arr, N);
        std::cout << __func__ << " size " << c.arr.size()
                              << " r " << r
                              << " exp " << c.exp
                              << " : " << (r == c.exp ? "ok" : "failed") << '\n';
    }
}

} // anonymous namespace

int main()
{
    test_missing_number();
    return 0;
}
