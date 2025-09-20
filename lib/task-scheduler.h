/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 */

#ifndef LIB_TASK_SCHEDULER_H
#define LIB_TASK_SCHEDULER_H

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <queue>
#include <string>
#include <thread>

constexpr auto sec = std::chrono::seconds(1);

struct TimerData {
    const void *user_data;
};

struct TaskScheduler final {
public:
    enum class Type {
        Oneshot = 1,
        Periodic,
    };

    struct Task {
        using Callback = std::function<void(const void *user_data)>;

        bool operator<(const Task &other) const;

        std::string m_name;
        std::chrono::steady_clock::duration m_timeout;
        std::chrono::steady_clock::time_point m_start_time;
        Type m_type;
        Callback m_func;
        const void *m_user_data;
    };

    TaskScheduler();
    ~TaskScheduler();

    bool add(const std::string &name,
             const TaskScheduler::Type type,
             const std::chrono::steady_clock::duration timeout,
             const Task::Callback func,
             const void *user_data);
    bool running() const;
    void start();
    void stop();

private:
    std::atomic<bool> m_running;
    std::condition_variable cv;
    std::mutex m_cv_lock;
    std::priority_queue<Task> m_pq;
    std::thread m_thread;
};

#endif // LIB_TASK_SCHEDULER_H
