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

#include <deque>
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

    std::vector<int> bfs_path_to_parent(const std::vector<int> &parent,
                                        int dst_idx,
                                        int src_idx)
    {
        std::vector<int> path;

        int idx = dst_idx;

        while (1) {
            path.push_back(idx);
            if (idx == src_idx) {
                return path;
            }

            if (idx == 0) {
                return std::vector<int>();
            }

            idx = parent[idx];
        }
    }

    /*
     * Breadth First Search
     *
     *   Init: Queue the source node and mark it as visited
     *
     *   1. If the queue is empty, quit
     *   2. If not, take a next node from the queue
     *   3. Add all neighbours of this node to the queue
     *   4. Mark them as visited
     *   5. Go to step 1
     */
    std::vector<int> bfs(const std::vector<std::vector<int>> &graph,
                         int src_idx,
                         int dst_idx)
    {
        if (!graph.size()) {
            return std::vector<int>();
        }

        std::vector<bool> visited(graph.size());
        std::vector<int> parent(graph.size());
        std::deque<int> queue;

        queue.push_back(src_idx);
        visited[src_idx] = true;

        while (!queue.empty()) {
            int idx = queue.front();
            queue.pop_front();
            std::cout << __func__ << " " << idx << '\n';

            for (const auto &n : graph[idx]) {
                if (!visited[n]) {
                    std::cout << __func__ << " add " << n << " to the queue" << '\n';
                    queue.push_back(n);
                    visited[n] = true;
                    parent[n] = idx;
                }
            }

            if (idx == dst_idx) {
                return bfs_path_to_parent(parent, dst_idx, src_idx);
            }

            if (debug) {
                std::cout << __func__ << "   visited " << ": " << show(visited) << '\n';
                std::cout << __func__ << "   queue for " << idx << ": " << show(queue) << '\n';
            }
        }
    }

    std::vector<std::vector<int>>
    dfs_from_to(const std::vector<std::vector<int>> &graph, int src, int dst)
    {
        std::cout << __func__ << " size " << graph.size() << " " << src << " " << dst << '\n';

        if (!graph.size()) {
            return std::vector<std::vector<int>>();
        }

        if (graph.size() == 1 && graph[0].size() == 0) {
            //std::vector<int> v;
            return { {}, };
        }

        std::vector<bool> visited(graph.size());
        std::vector<int> stackv;
        std::vector<int> stackn;
        std::vector<int> path;

        // Push the source and set its neighbour index to 0
        stackv.push_back(src);
        // Push neighbour idx == 0
        stackn.push_back(0);
        path.push_back(src);

        if (debug) {
            std::cout << __func__ << "   visited " << ": " << show(visited) << '\n';
            std::cout << __func__ << "   stackv " << ": " << show(stackv) << '\n';
            std::cout << __func__ << "   stackn " << ": " << show(stackn) << '\n';
            std::cout << __func__ << "   path " << ": " << show(path) << '\n';
            std::cout << __func__ << "   path == stackv " << (path == stackv ? "yes" : "no") << '\n';
        }

        std::vector<std::vector<int>> paths;
        while (!stackv.empty()) {
            //Vertex &v = stack.back();
            int &current_idx = stackv.back();
            int &neighbour_idx = stackn.back();

            //std::cout << __func__ << " " << v.idx << '\n';

            // if found destination or no more neighbours to explore
            if (current_idx == dst || neighbour_idx == (int)graph[current_idx].size()) {
                // yes, once again same if...
                if (current_idx == dst) {
                    if (debug) {
                        std::cout << __func__ << " found path from " << src
                                              << " to " << dst
                                              << ": " << show(path) << '\n';
                    }

                    paths.push_back(path);
                }

                visited[current_idx] = false;
                if (path.size() == 0) {
                    std::cout << __func__ << " attempt to pop from an empty path\n";
                    abort();
                }
                path.pop_back();
                stackv.pop_back();
                stackn.pop_back();
            } else {
                int next_neighbour = graph[current_idx][neighbour_idx];
                // increments neighbour_idx which points to the last element of the stackn
                ++neighbour_idx;
                if (!visited[next_neighbour]) {
                    visited[next_neighbour] = true;
                    stackv.push_back(next_neighbour);
                    // push next neighbour_idx == 0
                    stackn.push_back(0);
                    path.push_back(next_neighbour);
                }
            }

            if (debug) {
                std::cout << __func__ << " ---" << '\n';
                std::cout << __func__ << "   visited " << ": " << show(visited) << '\n';
                std::cout << __func__ << "   stackv for " << current_idx << ": " << show(stackv) << '\n';
                std::cout << __func__ << "   stackn for " << current_idx << ": " << show(stackn) << '\n';
                std::cout << __func__ << "   path for " << current_idx << ": " << show(path) << '\n';
                std::cout << __func__ << "   path == stackv " << (path == stackv ? "yes" : "no") << '\n';
            }
        }

        return paths;
    }

private:
#if 0
    struct Vertex {
        int current_idx;    // current vertex index
        int neighbour_idx;  // neighbour index
    };

    std::string show(const std::vector<Vertex> &v) const
    {
        std::string o;
        o += '(';
        for (const auto &e : v) {
            o += '{';
            o += std::to_string(e.current_idx);
            o += ',';
            o += std::to_string(e.neighbour_idx);
            o += '}';
            o += ',';
        }
        o += ')';

        return o;
    }
#endif

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

    std::string show(const std::deque<int> &q) const
    {
        std::string o;
        o += '(';
        for (const auto &e : q) {
            o += std::to_string(e);
            o += ',';
        }
        o += ')';

        return o;
    }

    //std::vector<int> current_path;
    //std::vector<std::vector<int>> simple_paths;
    bool debug = 10;
};

namespace {

static int test_dfs()
{
    const struct Case {
        //using namespace std;
        std::vector<std::vector<int>> g;
        std::vector<std::vector<int>> paths;
    } cases[] = {
        //{
        //    .g = { {}, },
        //    .paths = { {}, },
        //},
        //{
        //    .g = { {0,}, },
        //    .paths = { {0,}, },
        //},
        //           0    1    2    3
        //{ .g = {{1,2}, {3}, {3}, {}, }, },
        //
        //             0    1     2    3   4
        //{ .g = {{1,2,3}, {}, {3,4}, {}, {}, }, },
        //
        //           0        1    2   3   4
        //{ .g = {{1,3,4}, {2,3,4}, {3}, {}, {}, }, },
        //
        //           0        1    2    3   4
        //{ .g = {{1,3,4}, {2,3,4}, {3}, {4}, {}, }, },
        //
        //
        //{ .g = {{1,2}, {2,3}, {3,4}, {4}, {3}, }, },
        //
        //           0    1    2    3    4     5    6     7    8     9   10
        //{ .g = {{1,2,3}, {4}, {6}, {8}, {5}, {10}, {7}, {10}, {9}, {10}, {} }, },
        //
        //         0        1    2    3    4     5    6    7   8
        //{ .g = {{1}, {2,3,4}, {5}, {6}, {7},  {8}, {8}, {}, {}, }, },
        {
            // 2d mesh
            .g = {
                // 0      1    2      3      4     5    6    7    8
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

    const auto test_dfs_from_to = [&s](const Case &c) {
        std::vector<std::vector<int>> paths = s.dfs_from_to(c.g, 0, c.g.size() - 1);
        std::cout << "paths: " << paths.size() << '\n';
        //for (const auto &p : paths) {
        for (const auto &p : paths) {
            std::cout << '\t' << s.show(p) << '\n';
        }
        int r = paths == c.paths;
        std::cout << (r ? "ok" : "failed") << '\n';

        return r;
    };

    const auto test_bfs = [&s](const Case &c) {
        const std::vector<int> &p = s.bfs(c.g, 0, c.g.size() - 1);
        std::cout << "path " << s.show(p) << '\n';
        return 0;
    };

    for (const auto &c : cases) {
        //s.all_paths(c.g);
        //s.dfs(c.g);
        errors += test_bfs(c);
        //errors += !test_dfs_from_to(c);
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
