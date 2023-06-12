/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 *
 * Second Largest [1]
 *
 *      School     Accuracy: 26.72%     Submissions: 327K+     Points: 0
 *
 * Given an array Arr of size N, print second largest distinct element
 * from an array.
 *
 * Example 1:
 *
 *  Input:       N = 6, Arr[] = {12, 35, 1, 10, 34, 1}
 *
 *  Output:      34
 *
 *  Explanation: The largest element of the array is 35 and the second
 *               largest element is 34.
 *
 * Example 2:
 *
 *  Input:       N = 3, Arr[] = {10, 5, 10}
 *
 *  Output:      5
 *
 *  Explanation: The largest element of the array is 10 and the second
 *               largest element is 5.
 *
 * Your Task:
 *
 *      You don't need to read input or print anything. Your task is to
 *      complete the function print2largest() which takes the array of
 *      integers arr and n as parameters and returns an integer denoting the
 *      answer. If 2nd largest element doesn't exist then return -1.
 *
 * Expected Time Complexity: O(N)
 * Expected Auxiliary Space: O(1)
 *
 * Constraints:
 *
 *      2 <= N <= 10^5
 *      1 <= Arr_i <= 10^5
 *
 * [1] https://practice.geeksforgeeks.org/problems/second-largest3735/1
 *
 */

#include <climits>
#include <iostream>

#define ARRAY_SIZE(a)	(sizeof(a)/sizeof(a[0]))

namespace {

/*
 *  Function returns the second largest element
 *
 *  Hint 1:  Approach: Find the second largest element in a single
 *           traversal.  Below is the complete algorithm for doing this:
 *
 *  1) Initialize two variables first and second to INT_MIN as
 *     first = second = INT_MIN
 *  2) Start traversing the array,
 *     a) If the current element in array say arr[i] is greater
 *        than first. Then update first and second as,
 *          second = first
 *          first = arr[i]
 *     b) If the current element is in between first and second, then
 *        update second to store the value of current variable as
 *          second = arr[i]
 *  3) Return the value stored in second.
 */
static int print2largest(const int a[], int n)
{
    int max1 = INT_MIN;
    int max2 = INT_MIN;
    for (int i = 0; i < n; ++i) {
        if (max1 < a[i]) {
            max2 = max1;
            max1 = a[i];
        } else if (max2 < a[i] && a[i] < max1) {
            max2 = a[i];
        }
    }

    if (max2 != INT_MIN) {
        return max2;
    }

    return -1;
}

static void test_second_largest()
{

    const int arr1[] = { 12, 35, 1, 10, 34, 1 };
    const int arr2[] = { 10, 5, 10 };
    const int arr3[] = {
                         28, 188, 329, 327, 72, 33, 130, 270, 110, 150, 335,
                         166, 132, 123, 92, 331, 222, 293, 55, 302, 221, 281,
                         230, 172, 217, 109, 154, 247, 334, 256, 195, 36, 297,
                         283, 182, 175, 332, 45, 316, 206, 37, 227, 314, 174,
                         75, 322, 319, 120, 264, 103, 179, 74, 300, 165, 299,
                         324, 213, 290, 162, 177, 79, 34, 338, 192, 156, 180,
                         73, 301, 223, 276, 114, 232, 52, 155, 157, 268, 95,
                         304, 159, 160, 163, 214, 47, 25, 246, 243, 191, 124,
                         50, 279, 101, 63, 70, 347, 39, 149, 184, 62, 185, 77,
                         225, 196, 269, 202, 125, 41, 280, 215, 210, 266, 310,
                         197, 343, 51, 234, 53, 119, 272, 48, 250, 143, 326,
                         287, 84, 139, 56, 118, 323, 207, 284, 32, 96, 292,
                         148, 286, 229, 83, 330, 209, 176, 193, 82, 76, 321,
                         105, 67, 265, 261, 142, 66, 260, 68, 57, 140, 58, 341,
                         27, 40, 183, 90, 311, 282, 346, 31, 198, 226, 134, 80,
                         337, 133, 152, 86, 255, 205, 94, 263, 274, 100, 78,
                         224, 71, 131, 325, 112, 170, 88, 254, 54, 212, 267,
                         248, 308, 219, 307, 87, 104, 305, 249, 278, 147, 153,
                         339, 333, 108, 129, 106, 107, 228, 49, 164, 38, 42,
                         313, 262, 336, 91, 208, 296, 277, 64, 190, 201, 113,
                         271, 245, 144, 60, 252, 244, 303, 178, 111, 291, 315,
                         115, 318, 204, 241, 128, 181, 294, 317, 126, 69, 220,
                         235, 116, 320, 29, 238, 173, 200, 169, 342, 289, 167,
                         30, 85, 251, 345, 189, 240, 253, 59, 135, 312, 194,
                         239, 65, 151, 136, 237, 46, 242, 102, 44, 122, 285,
                         127, 161, 273, 288, 187, 218, 121, 81, 138, 98, 203,
                         141, 344, 295, 233, 158, 171, 186, 99, 199, 236, 61,
                         258, 43, 275, 93, 168, 35, 259, 145, 146, 137, 211,
                         89, 328, 216, 340, 117, 97, 26, 231, 257, 306, 309,
                         298 };

    struct {
        const int *arr;
        int arr_size;
        int exp;
    } cases[] = {
        {
            .arr = arr1,
            .arr_size = ARRAY_SIZE(arr1),
            .exp = 34,
        },
        {
            .arr = arr2,
            .arr_size = ARRAY_SIZE(arr2),
            .exp = 5,
        },
        {
            .arr = arr3,
            .arr_size = ARRAY_SIZE(arr3),
            .exp = 346,
        },
    };

    for (const auto &c : cases) {
        int r = print2largest(c.arr, c.arr_size);
        std::cout << __func__ << " size " << c.arr_size
                              << " r " << r
                              << " exp " << c.exp
                              << " : " << (r == c.exp ? "ok" : "failed") << '\n';
    }
}

} // anonymous namespace

int main()
{
    test_second_largest();

    return 0;
}
