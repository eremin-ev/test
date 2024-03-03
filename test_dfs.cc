/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 *
 * Depth First Search or DFS for a Graph
 * https://www.geeksforgeeks.org/depth-first-search-or-dfs-for-a-graph/
 *
 * Introduction to Depth First Search Algorithm (DFS)
 * https://www.baeldung.com/cs/depth-first-search-intro
 *
 * Find All Simple Paths Between Two Vertices in a Graph
 * https://www.baeldung.com/cs/simple-paths-between-two-vertices
 *
 *
 * Finally:
 *
 * Non recursive DFS algorithm for simple paths between two points
 * https://stackoverflow.com/questions/35170956/non-recursive-dfs-algorithm-for-simple-paths-between-two-points
 *
 */

//#include <climits>
//#include <cstdint>
#include <iostream>
#include <vector>

class Solution {
public:
#if 0
    std::vector<std::vector<int>> all_paths(const std::vector<std::vector<int>> &graph)
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

        return {};
    }
#endif

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

            std::cout << __func__ << "   visited " << ": " << show(visited) << '\n';
            std::cout << __func__ << "   stack for " << v << ": " << show(stack) << '\n';
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

    void dfs_from_to(const std::vector<std::vector<int>> &graph, int src, int dst)
    {
        if (!graph.size()) {
            return;
        }

        //int v_idx = 0;
        std::vector<bool> visited(graph.size());
        //std::vector<int> stack;
        std::vector<Vertex> stack;
        std::vector<int> path;

        stack.push_back({src, 0});
        path.push_back(src);

        std::cout << __func__ << "   visited " << ": " << show(visited) << '\n';
        std::cout << __func__ << "   stack " << ": " << show(stack) << '\n';

        while (!stack.empty()) {
            Vertex &v = stack.back();

            //std::cout << __func__ << " " << v.idx << '\n';

            // if found destination or no more neighbours to eplore
            if (v.idx == dst || v.neighbour == (int)graph[v.idx].size()) {
                // yes, once again same if...
                if (v.idx == dst) {
                    std::cout << __func__ << " found path from " << src
                                          << " to " << dst
                                          << ": " << show(path) << '\n';
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

            //std::cout << __func__ << "   visited " << ": " << show(visited) << '\n';
            //std::cout << __func__ << "   stack for " << v.idx << ": " << show(stack) << '\n';
            //std::cout << __func__ << "   path for " << v.idx << ": " << show(path) << '\n';
        }
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
    std::vector<int> current_path;
    //std::vector<std::vector<int>> simple_paths;
};

namespace {

static int test_dfs()
{
    const struct {
        //using namespace std;
        std::vector<std::vector<int>> g;
        //uint32_t b;
    } cases[] = {
        //{ .g = {{1,2}, {3}, {3}, {}, }, },
        //{ .g = {{1,2,3}, {}, {3,4}, {}, {}, }, },
        //           0        1    2   3   4
        //{ .g = {{1,3,4}, {2,3,4}, {3}, {}, {}, }, },
        //           0        1    2    3   4
        //{ .g = {{1,3,4}, {2,3,4}, {3}, {4}, {}, }, },
        //{ .g = {{1,2}, {2,3}, {3,4}, {4}, {3}, }, },
        //           0    1    2    3    4     5    6     7    8     9   10
        { .g = {{1,2,3}, {4}, {6}, {8}, {5}, {10}, {7}, {10}, {9}, {10}, {} }, },
        //       0        1    2    3    4     5    6    7   8
        //{ .g = {{1}, {2,3,4}, {5}, {6}, {7},  {8}, {8}, {}, {}, }, },
    };

    int errors = 0;
    Solution s;

    for (const auto &c : cases) {
        int r = 0;
        //s.all_paths(c.g);
        //s.dfs(c.g);
        s.dfs_from_to(c.g, 0, c.g.size() - 1);
        std::cout << (r ? "ok" : "failed") << " "
                  << '\n';
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
