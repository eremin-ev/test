/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 *
 * Unique rows in boolean matrix [1]
 *
 *      Easy    Accuracy: 48.36%    Submissions: 20K+   Points: 2
 *
 * Given a binary matrix your task is to find all unique rows of
 * the given matrix in the order of their appearance in the matrix.
 *
 * Example 1:
 *
 *  Input:  row = 3, col = 4, M[][] = { { 1 1 0 1 },
 *                                      { 1 0 0 1 },
 *                                      { 1 1 0 1 } }
 *
 *  Output: { 1 1 0 1 }, { 1 0 0 1 }
 *
 *  Explanation:
 *
 *      Above the matrix of size 3x4 looks like
 *
 *          1 1 0 1
 *          1 0 0 1
 *          1 1 0 1
 *
 *      The two unique rows are R1: {1 1 0 1} and R2: {1 0 0 1}.
 *      As R1 first appeared at row-0 and R2 appeared at row-1,
 *      in the resulting list, R1 is kept before R2.
 *
 * Example 2:
 *
 *  Input: row = 2, col = 4, M[][] = { { 0 0 0 1 },
 *                                     { 0 0 0 1 } }
 *
 *  Output: { 0 0 0 1 }
 *
 *  Explanation:
 *
 *      Above the matrix of size 3x4 looks like
 *
 *          0 0 0 1
 *          0 0 0 1
 *
 *      Only unique row is { 0 0 0 1 }
 *
 * Your Task:
 *
 *      You only need to implement the given function uniqueRow().
 *      The function takes three arguments the first argument is a
 *      matrix M and the next two arguments are row and col denoting
 *      the rows and columns of the matrix. The function should
 *      return the list of the unique row of the martrix. Do not
 *      read input, instead use the arguments given in the function.
 *
 * Note: The drivers code print the rows "$" separated. You have to
 * just return list of rows, you do not have to worry about printing
 * anything.
 *
 * Expected Time Complexity: O(row * col)
 * Expected Auxiliary Space: O(row * col)
 *
 * Constraints:
 *
 *      1 <= row,col <= 40,
 *      0 <= M[][] <= 1
 *
 * [1] https://practice.geeksforgeeks.org/problems/unique-rows-in-boolean-matrix/1
 */

#include "lib/util.h"

#include <iostream>
#include <vector>
#include <unordered_set>

#define MAX 1000

namespace {

/*
 * Hash function for vector<int>, see [1, 2].
 *
 * [1] C++ unordered_set of vectors
 *     https://stackoverflow.com/questions/29855908/c-unordered-set-of-vectors
 *
 * [2] GeeksforGeeks: unordered set of Vectors in C++ with Examples
 *     https://www.geeksforgeeks.org/unordered-set-of-vectors-in-c-with-examples/
 */
struct VectorHash {
    size_t operator()(const std::vector<int> &v) const
    {
        std::hash<int> hasher;
        size_t seed = 0;

        for (int i : v) {
            seed ^= hasher(i) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        }

        return seed;
    }
};

static void matrix_print(const int M[MAX][MAX], int rows, int cols)
{
	for (int r = 0; r < rows; ++r) {
		for (int c = 0; c < cols; ++c) {
    		std::cout << '\t' << M[r][c];
        }
		std::cout << '\n';
	}
    std::cout << '\n';
}

static void matrix_prepare(int M[MAX][MAX], const int *m, int rows, int cols)
{
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            M[r][c] = m[r*cols + c];
        }
    }
}

static std::vector<std::vector<int>> uniqueRow(const int M[MAX][MAX], int rows, int cols)
{
    std::vector<std::vector<int>> out;
    std::unordered_set<std::vector<int>, VectorHash> vs;
    for (int r = 0; r < rows; ++r) {
        std::vector<int> v;
        for (int c = 0; c < cols; ++c) {
            v.push_back(M[r][c]);
        }

        const auto i = vs.find(v);
        if (i == vs.end()) {
            out.push_back(v);
            vs.insert(v);
        }
    }
    return out;
}

static void test_uniq_rows_in_bool_matrix()
{
    const int M1[] = { 1, 1, 0, 1,
                       1, 0, 0, 1,
                       1, 1, 0, 1 };
    const int M2[] = { 0, 0, 0, 1,
                       0, 0, 0, 1 };
    struct {
        const int *m;
        int rows;
        int cols;
        std::vector<std::vector<int>> exp;
    } cases[] = {
        {
            .m = M1,
            .rows = 3,
            .cols = 4,
            .exp = { { 1, 1, 0, 1 }, { 1, 0, 0, 1 } },
        },
        {
            .m = M2,
            .rows = 2,
            .cols = 4,
            .exp = { { 0, 0, 0, 1 } },
        },
    };

    int M[MAX][MAX];

    for (const auto &c : cases) {
        matrix_prepare(M, c.m, c.rows, c.cols);
        matrix_print(M, c.rows, c.cols);
        const std::vector<std::vector<int>> r = uniqueRow(M, c.rows, c.cols);
        for (const auto &el : r) {
            std::cout << __func__ << " " << el << '\n';
        }
        std::cout << __func__ << " Rows x Cols " << c.rows << " x " << c.cols
                              //<< " r " << r
                              //<< " exp " << c.exp
                              << " : " << (r == c.exp ? "ok" : "failed") << '\n';
    }
}

} // anonymous namespace

int main()
{
    test_uniq_rows_in_bool_matrix();
    return 0;
}
