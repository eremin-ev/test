/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 *
 *
 * 797. All Paths From Source to Target [1]
 *
 *   Medium
 *
 *   Topics: Backtracking, Depth-First Search, Breadth-First Search, Graph
 *
 *   Companies: ?
 *
 * Given a directed acyclic graph (DAG) of n nodes labeled from 0 to
 * n - 1, find all possible paths from node 0 to node n - 1 and return
 * them in any order.
 *
 * The graph is given as follows: graph[i] is a list of all nodes you
 * can visit from node i (i.e., there is a directed edge from node i to
 * node graph[i][j]).
 *
 *
 * Example 1:
 *
 *
 *   0  -->  1
 *
 *   |       |
 *   v       v
 *
 *   2  -->  3
 *
 *  Input: graph = [[1,2],[3],[3],[]]
 *  Output: [[0,1,3],[0,2,3]]
 *  Explanation: There are two paths: 0 -> 1 -> 3 and 0 -> 2 -> 3.
 *
 *
 * Example 2:
 *
 *  ________________
 *  |               \
 *  |      0  ---->  1
 *  |     / \       / \
 *  \    v   v     v   v
 *   -> 4     3   3     2
 *
 *  Input: graph = [[4,3,1],[3,2,4],[3],[4],[]]
 *  Output: [[0,4],[0,3,4],[0,1,3,4],[0,1,2,3,4],[0,1,4]]
 *
 *
 * Constraints:
 *
 *   n == graph.length
 *   2 <= n <= 15
 *   0 <= graph[i][j] < n
 *   graph[i][j] != i (i.e., there will be no self-loops).
 *   All the elements of graph[i] are unique.
 *   The input graph is guaranteed to be a DAG (directed acyclic graph).
 *
 *
 * Result
 *
 *      Accepted
 *
 *          https://leetcode.com/problems/all-paths-from-source-to-target/submissions/1194036268/
 *
 *      Evgeny Eremin submitted at Mar 05, 2024 01:00
 *
 *      Runtime         7ms     Beats   84.11% of users with C++
 *
 *      Memory      11.93MB     Beats   82.52% of users with C++
 *
 *
 * References
 *
 * [1] 797. All Paths From Source to Target [1]
 *     https://leetcode.com/problems/all-paths-from-source-to-target/description/
 *
 * [2] Depth First Search or DFS for a Graph
 *     https://www.geeksforgeeks.org/depth-first-search-or-dfs-for-a-graph/
 *
 * [3] Introduction to Depth First Search Algorithm (DFS)
 *     https://www.baeldung.com/cs/depth-first-search-intro
 *
 * [4] Find All Simple Paths Between Two Vertices in a Graph
 *     https://www.baeldung.com/cs/simple-paths-between-two-vertices
 *
 * [5] Non recursive DFS algorithm for simple paths between two points
 *     https://stackoverflow.com/questions/35170956/non-recursive-dfs-algorithm-for-simple-paths-between-two-points
 *
 */

#include <iostream>
#include <vector>

class Solution {
public:
    std::string show(const std::vector<int> &v) const
    {
        std::string o;
        o += '(';
        for (const auto &e : v) {
            o += std::to_string(e);
            o += ',';
        }
        o += ')';

        return o;
    }

    void show_neighbours(const std::vector<std::vector<int>> &graph)
    {
        int v_idx = 0;
        std::vector<bool> visited(graph.size());
        //std::cout << __func__ << "visited " << visited << '\n';
        for (const auto &v : graph) {
            std::cout << __func__ << " neighbours of " << v_idx << '\n';
            for (const auto &n : v) {
                std::cout << __func__ << "  " << n << '\n';
            }
            ++v_idx;
        }
    }

    void dfs(const std::vector<std::vector<int>> &graph)
    {
        if (!graph.size()) {
            return;
        }

        //int v_idx = 0;
        std::vector<bool> visited(graph.size());
        std::vector<int> stack;

        stack.push_back(0);

        while (!stack.empty()) {
            int v = stack.back();
            stack.pop_back();
            std::cout << __func__ << " " << v << '\n';
            visited[v] = true;
            for (const auto &n : graph[v]) {
                //std::cout << __func__ << "  " << n << '\n';
                if (!visited[n]) {
                    stack.push_back(n);
                }
            }

            if (debug) {
                std::cout << __func__ << "   visited " << ": " << show(visited) << '\n';
                std::cout << __func__ << "   stack for " << v << ": " << show(stack) << '\n';
            }
        }

        //std::cout << __func__ << "visited " << visited << '\n';
        /*for (const auto &v : graph) {
            std::cout << __func__ << " neighbours of " << v_idx << '\n';
            for (const auto &n : v) {
                std::cout << __func__ << "  " << n << '\n';
            }
            ++v_idx;
        }*/
    }

    std::vector<std::vector<int>>
    dfs_from_to(const std::vector<std::vector<int>> &graph, int src, int dst)
    {
        if (!graph.size()) {
            return std::vector<std::vector<int>>();
        }

        std::vector<bool> visited(graph.size());
        std::vector<Vertex> stack;
        std::vector<int> path;

        stack.push_back({src, 0});
        path.push_back(src);

        if (debug) {
            std::cout << __func__ << "   visited " << ": " << show(visited) << '\n';
            std::cout << __func__ << "   stack " << ": " << show(stack) << '\n';
        }

        std::vector<std::vector<int>> paths;
        while (!stack.empty()) {
            Vertex &v = stack.back();

            //std::cout << __func__ << " " << v.idx << '\n';

            // if found destination or no more neighbours to eplore
            if (v.idx == dst || v.neighbour == (int)graph[v.idx].size()) {
                // yes, once again same if...
                if (v.idx == dst) {
                    if (debug) {
                        std::cout << __func__ << " found path from " << src
                                              << " to " << dst
                                              << ": " << show(path) << '\n';
                    }

                    paths.push_back(path);
                }

                visited[v.idx] = false;
                if (path.size() == 0) {
                    std::cout << __func__ << " attempt to pop from an empty path\n";
                    abort();
                }
                path.pop_back();
                stack.pop_back();
            } else {
                int next_neighbour = graph[v.idx][v.neighbour];
                ++v.neighbour;
                if (!visited[next_neighbour]) {
                    visited[next_neighbour] = true;
                    stack.push_back({next_neighbour, 0});
                    path.push_back(next_neighbour);
                }
            }

            if (debug) {
                std::cout << __func__ << "   visited " << ": " << show(visited) << '\n';
                std::cout << __func__ << "   stack for " << v.idx << ": " << show(stack) << '\n';
                std::cout << __func__ << "   path for " << v.idx << ": " << show(path) << '\n';
            }
        }

        return paths;
    }

private:
    struct Vertex {
        int idx;
        int neighbour;  // neighbour index
    };

    std::string show(const std::vector<Vertex> &v) const
    {
        std::string o;
        o += '(';
        for (const auto &e : v) {
            o += '{';
            o += std::to_string(e.idx);
            o += ',';
            o += std::to_string(e.neighbour);
            o += '}';
            o += ',';
        }
        o += ')';

        return o;
    }

    std::string show(const std::vector<bool> &v) const
    {
        std::string o;
        o += '(';
        int idx = 0;
        for (const auto &e : v) {
            o += std::to_string(idx);
            o += ':';
            o += e ? "yes" : "no";
            o += ',';
            ++idx;
        }
        o += ')';

        return o;
    }
    //std::vector<int> current_path;
    //std::vector<std::vector<int>> simple_paths;
    bool debug = 0;
};

namespace {

static int test_dfs()
{
    const struct {
        //using namespace std;
        std::vector<std::vector<int>> g;
        std::vector<std::vector<int>> paths;
    } cases[] = {
        {
            .g = { {}, },
            .paths = { {}, },
        },
        //{ .g = {{1,2}, {3}, {3}, {}, }, },
        //{ .g = {{1,2,3}, {}, {3,4}, {}, {}, }, },
        //           0        1    2   3   4
        //{ .g = {{1,3,4}, {2,3,4}, {3}, {}, {}, }, },
        //           0        1    2    3   4
        //{ .g = {{1,3,4}, {2,3,4}, {3}, {4}, {}, }, },
        //{ .g = {{1,2}, {2,3}, {3,4}, {4}, {3}, }, },
        //           0    1    2    3    4     5    6     7    8     9   10
        //{ .g = {{1,2,3}, {4}, {6}, {8}, {5}, {10}, {7}, {10}, {9}, {10}, {} }, },
        //       0        1    2    3    4     5    6    7   8
        //{ .g = {{1}, {2,3,4}, {5}, {6}, {7},  {8}, {8}, {}, {}, }, },
        // 2d mesh  0       1    2      3      4     5    6    7    8
        {   .g = {
                {1,3}, {2,4}, {5}, {4,6}, {5,7},  {8}, {7}, {8}, {},
            },
            .paths = {
                {0,1,2,5,8,},
                {0,1,4,5,8,},
                {0,1,4,7,8,},
                {0,3,4,5,8,},
                {0,3,4,7,8,},
                {0,3,6,7,8,},
            },
        },
    };

    int errors = 0;
    Solution s;

    for (const auto &c : cases) {
        //s.all_paths(c.g);
        //s.dfs(c.g);
        std::vector<std::vector<int>> paths = s.dfs_from_to(c.g, 0, c.g.size() - 1);
        std::cout << "paths:\n";
        for (const auto &p : paths) {
            std::cout << '\t' << s.show(p) << '\n';
        }
        int r = paths == c.paths;
        std::cout << (r ? "ok" : "failed") << '\n';
        errors += !r;
    }

    return errors;
}

} // anonymous namespace

int main()
{
    int errors = 0;

    errors += test_dfs();

    std::cout << "errors " << errors << '\n';

    return 0;
}
