/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 *
 * Task Scheduler [1]
 *
 *      Medium  Accuracy: 58.08%    Submissions: 26K+   Points: 4
 *
 * Given a character array tasks of size N, representing the tasks a CPU needs
 * to do, where each letter represents a different task.  Tasks could be done
 * in any order. Each task is done in one unit of time. For each unit of time,
 * the CPU could complete either one task or just be idle.
 *
 * CPU has a cooldown period for each task. CPU can repeat a task only after
 * atleast K units of time has passed after it did same task last time. It can
 * perform other tasks in that time, can stay idle to wait for repeating that
 * task.
 *
 * Return the least number of units of times that the CPU will take to finish
 * all the given tasks.
 *
 * Example 1:
 *
 *  Input:  N = 6, K = 2, task[] = { 'A', 'A', 'A', 'B', 'B', 'B' }
 *
 *  Output: 8
 *
 *  Explanation:
 *
 *      A -> B -> idle -> A -> B -> idle -> A -> B
 *
 *      There is atleast 2 units of time between any two same tasks.
 *
 * Example 2:
 *
 *  Input:  N = 12, K = 2,
 *          task[ ] = {'A', 'A', 'A', 'A', 'A', 'A', 'B', 'C', 'D', 'E', 'F', 'G'}
 *
 *  Output: 16
 *
 *  Explanation:
 *
 *      One possible solution is
 *
 *      A -> B -> C -> A -> D -> E -> A -> F -> G -> A -> idle -> idle -> A -> idle -> idle -> A.
 *
 * Your Task:
 *
 *      You don't need to read input or print anything. Your task is to
 *      complete the function leastInterval() which takes the interger N,
 *      integer K and an character array tasks as parameters and returns the
 *      minimum unit of time required to complete all tasks.
 *
 * Expected Time Complexity: O(N)
 * Expected Auxiliary Space: O(1)
 *
 * Constraints:
 *
 *      1 <= N <= 10^4
 *      0 <= K <= 100
 *      task_i is upper-case English letter.
 *
 * [1] https://practice.geeksforgeeks.org/problems/task-scheduler/1
 *
 */

#include <algorithm>
#include <iostream>
#include <vector>

//#define DEBUG
//#define DEBUG2
//#define DEBUG3

namespace {

struct task_info {
    char id;
    int cnt;
    int delay;
};

static void count_tasks(const std::vector<char> &tasks, task_info *ti, int ti_nr)
{
    for (unsigned char t : tasks) {
        ++ti[t - 'A'].cnt;
    }

    for (int t = 0; t < ti_nr; ++t) {
        ti[t].id = 'A' + t;
#ifdef DEBUG3
        std::cout << __func__ << " " << ti[t].id << " " << ti[t].cnt << '\n';
#endif
    }
}

//
// Return value:
//
//   -1 if all are delayed
//   -2 if no tasks left
//
static int find_next(int K, task_info *ti, int ti_nr)
{
    bool found_waiting = false;
    bool found_ready = false;
    int cnt_max = 0;
    int t_ready;
    for (int t = 0; t < ti_nr; ++t) {
        if (ti[t].cnt) {
            found_waiting = true;
#ifdef DEBUG2
            std::cout << __func__ << "    " << ti[t].id
                                  << " cnt " << ti[t].cnt
                                  << " delay " << ti[t].delay
                                  << '\n';
#endif
            if (ti[t].delay) {
                --ti[t].delay;
            } else if (cnt_max < ti[t].cnt) {
                cnt_max = ti[t].cnt;
                t_ready = t;
                found_ready = true;
            }
        }
    }

    if (found_ready) {
        ti[t_ready].delay = K;
        --ti[t_ready].cnt;
        return t_ready;
    }

    if (found_waiting) {
        return -1;
    }

    return -2;
}

static int count_ops(int K, task_info *ti, int ti_nr)
{
    int ops = 0;

#ifdef DEBUG
    for (int t = 0; t < ti_nr; ++t) {
        std::cout << __func__ << " " << ti[t].id
                              << " " << ti[t].cnt
                              << " " << ti[t].delay << '\n';
    }
#endif

    while (true) {
        int t_max = find_next(K, ti, ti_nr);
        if (t_max == -1) {
#ifdef DEBUG
            std::cout << __func__ << " t_max -1, delay" << '\n';
#endif
            ++ops;
        } else if (t_max == -2) {
#ifdef DEBUG
            std::cout << __func__ << " -2, end" << '\n';
#endif
            break;
        } else {
#ifdef DEBUG
            std::cout << __func__ << " issuing " << ti[t_max].id << '\n';
#endif
            ++ops;
        }
    }

    return ops;
}

static int leastInterval1(int, int K, const std::vector<char> &tasks)
{
    constexpr int ti_nr = 'Z' - 'A' + 1;
    task_info ti[ti_nr] = { 0, 0, 0 };

    count_tasks(tasks, ti, ti_nr);

    //std::sort(std::begin(ti), std::end(ti), [](const task_info &a, const task_info &b) {
    //    return a.cnt > b.cnt;
    //});

    return count_ops(K, ti, ti_nr);
}

/*
 * Hint 1: Which task will result in more idling of the CPU?
 *
 * Hint 2: Either all tasks could be completed without CPU staying idle
 *         or a task with maximum occurence will force CPU to remain idle
 *         after every other task is compelted
 */
static int leastInterval2(int N, int K, const std::vector<char> &tasks)
{
    constexpr int nr = 'Z' - 'A' + 1;
    int freq[nr] = { 0 };
    int maxf = 0;

    for (const auto t : tasks) {
        int idx = t - 'A';
        ++freq[idx];
        maxf = std::max(maxf, freq[idx]);
    }

    int maxfcount = 0;
    for (int i = 0; i < nr; i++) {
        if (freq[i] == maxf) {
            maxfcount++;
        }
    }

    int time = (maxf - 1)*(K + 1) + maxfcount;
    int time = (maxf - 1)*K + (maxf - 1) + maxfcount;

    return std::max(time, N);
}

} // anonymous namespace

int main()
{
    struct {
        std::vector<char> tasks;
        int K;
        int exp;
    } cases[] = {
        {
            .tasks = { 'A', 'A', 'A' },
            .K = 0,
            .exp = 3,
        },

        {
            .tasks = { 'A', 'A', 'A' },
            .K = 1,
            .exp = 5,
        },
        {
            .tasks = { 'A', 'A', 'A', 'B', 'B', 'B' },
            .K = 2,
            .exp = 8,
        },
        {
            .tasks = { 'A', 'A', 'A', 'A', 'A', 'A', 'B', 'C', 'D', 'E', 'F', 'G' },
            .K = 2,
            .exp = 16,
        },
        {
            .tasks = { 'Z' },
            .K = 0,
            .exp = 1,
        },

        {
            .tasks = { 'Q', 'G', 'H', 'U', 'M', 'E', 'A', 'Y', 'L', 'N',
                       'L', 'F', 'D', 'X', 'F', 'I', 'R', 'C', 'V', 'S',
                       'C', 'X', 'G', 'G', 'B', },
            .K = 17,
            .exp = 37,
        },
        {
            .tasks = { 'Q', 'G', 'H', 'U', 'M', 'E', 'A', 'Y', 'L', 'N',
                       'L', 'F', 'D', 'X', 'F', 'I', 'R', 'C', 'V', 'S',
                       'C', 'X', 'G', 'G', 'B', 'W', 'K', 'F', 'N', 'Q',
                       'D', 'U', 'X', 'W', 'F', 'N', 'F', 'O', 'Z', 'V',
                       'S', 'R', },
            .K = 17,
            .exp = 73,
        },
        {
            .tasks = { 'Q', 'G', 'H', 'U', 'M', 'E', 'A', 'Y', 'L', 'N',
                       'L',  },
            .K = 7,
            .exp = 11,
        },

        {
            .tasks = {
                       'W', 'W', 'W', 'W', 'W', 'W', 'W', 'W', 'W', 'W',
                       'W', 'W', 'W', 'W', 'W', 'W', 'W', 'W', 'W', 'W',
                       'S', 'S', 'S', 'S', 'S', 'S', 'S', 'S', 'S', 'S',
                       'S', 'S', 'S', 'S', 'S', 'S', 'S', 'S', 'S', 'S',
                       'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A',
                       'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A',
                       'Z', 'Z', 'Z', 'Z', 'Z', 'Z', },
            .K = 16,
            .exp = 326,
        },

        {
            .tasks = { 'H', 'L', 'V', 'B', 'Y', 'P', 'E', 'C', 'J', 'E',
                       'I', 'W', 'W', 'S', 'T', 'N', 'E', 'U', 'I', 'Q',
                       'U', 'B', 'P', 'G', 'U', 'G', 'W', 'G', 'N', 'M',
                       'I', 'K', 'D', 'W', 'I', 'C', 'K', 'D', 'I', 'L',
                       'S', 'N', 'P', 'D', 'Q', 'S', 'V', 'M', 'T', 'E',
                       'R', 'D', 'F', 'B', 'L', 'H', 'W', 'B', 'A', 'F',
                       'B', 'G', 'A', 'R', 'C', 'S', 'F', 'I', 'J', 'Y',
                       'L', 'B', 'I', 'I', 'M', 'O', 'F', 'G', 'X', 'E',
                       'I', 'R', 'P', 'B', 'T', 'Y', 'W', 'Q', 'Z', 'A',
                       'P', 'K', 'K', 'C', 'A', 'F', 'C', 'O', 'M', 'V',
                       'N', 'T', 'M', 'D', 'H', 'Z', 'X', 'L', 'E', 'F',
                       'P', 'P', 'Y', 'E', 'B', 'E', 'O', 'E', 'Q', 'Z',
                       'N', 'B', 'H', 'Y', 'C', 'W', 'M', 'F', 'Y', 'A',
                       'Q', 'Q', 'I', 'A', 'N', 'N', 'Y', 'H', 'Q', 'O',
                       'U', 'I', 'Z', 'V', 'E', 'A', 'V', 'G', 'Z', 'J',
                       'W', 'O', 'B', 'L', 'S', 'G', 'W', 'N', 'K', 'V',
                       'J', 'L', 'N', 'I', 'V', 'Q', 'C', 'W', 'V', 'Z',
                       'X', 'T', 'C', 'T', 'H', 'Y', 'N', 'M', 'U', 'X',
                       'Q', 'K', 'W', 'W', 'M', 'T', 'F', },
            .K = 16,
            .exp = 189,
        },

        {
            .tasks = { 'H', 'L', 'V', 'B', 'Y', 'P', 'E', 'C', 'J', 'E',
                       'I', 'W', 'W', 'S', 'T', 'N', 'E', 'U', 'I', 'Q',
                       'U', 'B', 'P', 'G', 'U', 'G', 'W', 'G', 'N', 'M',
                       'I', 'K', 'D', 'W', 'I', 'C', 'K', 'D', 'I', 'L',
                       'S', 'N', 'P', 'D', 'Q', 'S', 'V', 'M', 'T', 'E',
                       'R', 'D', 'F', 'B', 'L', 'H', 'W', 'B', 'A', 'F',
                       'B', 'G', 'A', 'R', 'C', 'S', 'F', 'I', 'J', 'Y',
                       'L', 'B', 'I', 'I', 'M', 'O', 'F', 'G', 'X', 'E',
                       'I', 'R', 'P', 'B', 'T', 'Y', 'W', 'Q', 'Z', 'A',
                       'P', 'K', 'K', 'C', 'A', 'F', 'C', 'O', 'M', 'V',
                       'N', 'T', 'M', 'D', 'H', 'Z', 'X', 'L', 'E', 'F',
                       'P', 'P', 'Y', 'E', 'B', 'E', 'O', 'E', 'Q', 'Z', },
            .K = 16,
            .exp = 138,
        },
    };

    for (const auto &c : cases) {
        int r = leastInterval1(c.tasks.size(), c.K, c.tasks);
        std::cout << c.tasks.size() << " -> "
                  << r << " <?> " << c.exp << " : "
                  << (r == c.exp ? "ok" : "failed" ) << '\n';
    }

    for (const auto &c : cases) {
        int r = leastInterval2(c.tasks.size(), c.K, c.tasks);
        std::cout << c.tasks.size() << " -> "
                  << r << " <?> " << c.exp << " : "
                  << (r == c.exp ? "ok" : "failed" ) << '\n';
    }

    return 0;
}
