/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 *
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

private:
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
        { .g = {{1,2}, {3}, {3}, {}}, },
        { .g = {{1,2,3}, {}, {3,4}, {}, {}}, },
        { .g = {{1,2}, {2,3}, {3,4}, {4}, {3}}, },
    };

    int errors = 0;
    Solution s;

    for (const auto &c : cases) {
        int r = 0;
        //s.all_paths(c.g);
        s.dfs(c.g);
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
