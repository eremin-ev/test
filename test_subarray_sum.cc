/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 *
 * Subarray with given sum [1]
 *
 *    Easy    Accuracy: 16.5%    Submissions: 1.2M    Points: 2
 *
 * Given an unsorted array A of size N that contains only positive
 * integers, find a continuous sub-array that adds to a given number S
 * and return the left and right index (1-based indexing) of that
 * subarray.
 *
 * In case of multiple subarrays, return the subarray indexes which
 * come first on moving from left to right.
 *
 * Note: You have to return an ArrayList consisting of two elements
 *       left and right. In case no such subarray exists return an
 *       array consisting of element -1.
 *
 * Example 1:
 *
 *  Input: N = 5, S = 12, A[] = { 1, 2, 3, 7, 5 }
 *
 *  Output: { 2, 4 }
 *
 *  Explanation: The sum of elements from 2nd position to 4th position
 *               is 12.
 *
 * Example 2:
 *
 *  Input: N = 10, S = 15, A[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 }
 *
 *  Output: { 1, 5 }
 *
 *  Explanation: The sum of elements from 1st position to 5th position
 *               is 15.
 *
 * Your Task:
 *
 *      You don't need to read input or print anything. The task is to
 *      complete the function subarraySum() which takes arr, N, and S as
 *      input parameters and returns an ArrayList containing the starting
 *      and ending positions of the first such occurring subarray from
 *      the left where sum equals to S. The two indexes in the array
 *      should be according to 1-based indexing. If no such subarray is
 *      found, return an array consisting of only one element that is -1.
 *
 * Expected Time Complexity: O(N)
 * Expected Auxiliary Space: O(1)
 *
 * Constraints:
 *
 *      1 <= N <= 10^5
 *      1 <= A_i <= 10^9
 *      0 <= S <= 10^{14}
 *
 * [1] https://practice.geeksforgeeks.org/problems/subarray-with-given-sum-1587115621/1
 */

#include <iostream>
#include <iterator>
#include <vector>

namespace {

template <typename T>
std::ostream &operator<<(std::ostream &os, const std::vector<T> &v)
{
    std::ostream_iterator<T> oit(std::cout, ", ");
    //*oit = '{';
    std::copy(v.begin(), v.end(), oit);
    //*oit = '}';
    return os;
}

// Function to find a continuous sub-array which adds up to a given number.
static std::vector<int> subarraySum(const std::vector<int> A, int N, long long S)
{
    if (N == 0) {
        return {-1};
    }

    long long s1 = A[0];
    for (int i = 0, j = 0; ; ) {
        bool add_j = false;
        if (s1 < S) {
            j++;
            add_j = true;
        } else if (s1 > S) {
            s1 -= A[i];
            i++;
        }

        if (i >= N || j >= N) {
            break;
        }

        if (i > j) {
            j = i;
            s1 = A[i];
        } else {
            if (add_j) {
                s1 += A[j];
            }
        }

        if (s1 == S) {
            return {i + 1, j + 1};
        }
    }

    return {-1};
}

static void test_subarray_sum()
{
    struct {
        const std::vector<int> A;
        long long S;
        std::vector<int> exp;
    } cases[] = {
        {
            .A = { 1, 2, 3, 7, 5 },
            .S = 12,
            .exp = { 2, 4 },
        },
        {
            .A = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 },
            .S = 15,
            .exp = { 1, 5 },
        },
    };

    for (const auto &c : cases) {
        const std::vector<int> r = subarraySum(c.A, c.A.size(), c.S);
        std::cout << __func__ << " size " << c.A.size()
                              << " r " << r
                              << " exp " << c.exp
                              << " : " << (r == c.exp ? "ok" : "failed") << '\n';
    }
}

} // anonymous namespace

int main()
{
    test_subarray_sum();
    return 0;
}
