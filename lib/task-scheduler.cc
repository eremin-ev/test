/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 */

#include "task-scheduler.h"

#include <thread>
#include <iostream>

namespace {

std::string time_point_to_str(const std::chrono::steady_clock::time_point tp)
{
    const auto since_epoch = tp.time_since_epoch();
    long long sec = std::chrono::duration_cast<std::chrono::seconds>(since_epoch).count();
    return std::to_string(sec) + " sec";
}

std::string duration_to_str(const std::chrono::steady_clock::duration d)
{
    const long long ms = std::chrono::duration_cast<std::chrono::milliseconds>(d).count();
    return std::to_string(ms) + " ms";
}

} // namespace

bool TaskScheduler::Task::operator<(const Task &other) const
{
    // "Priority" is inverted so the heap top is a task with the least start time
    return other.m_start_time < m_start_time;
}

TaskScheduler::TaskScheduler()
    : m_running(false)
{
}

TaskScheduler::~TaskScheduler()
{
    stop();
}

bool TaskScheduler::running() const
{
    return m_running;
}

bool TaskScheduler::add(const std::string &name,
                        const TaskScheduler::Type type,
                        const std::chrono::steady_clock::duration timeout,
                        const TaskScheduler::Task::Callback func,
                        const void *user_data)
{
    const std::chrono::steady_clock::time_point start_time = std::chrono::steady_clock::now() + timeout;

    std::cout << __func__ << " add " << name << " at " << time_point_to_str(start_time) << '\n';

    m_pq.emplace(Task{
        .m_name = std::move(name),
        .m_timeout = timeout,
        .m_start_time = start_time,
        .m_type = type,
        .m_func = func,
        .m_user_data = user_data,
    });

    return true;
}

void TaskScheduler::start()
{
    if (m_running)
        return;

    m_running = true;

    m_thread = std::thread([this] {
        while (m_running) {
            std::unique_lock<std::mutex> lock(m_cv_lock);

            if (m_pq.empty()) {
                std::cout << __func__ << " no tasks, waiting for pq change ... " << '\n';
                cv.wait(lock, [this] { return !m_running || !m_pq.empty(); });
            }

            if (!m_running)
                return;

            // Ok, we have a task in the pq
            const auto now1 = std::chrono::steady_clock::now();
            const auto next_wakeup = m_pq.top().m_start_time;
            std::cout << __func__ << " next task " << m_pq.top().m_name
                      << " now1 " << time_point_to_str(now1)
                      << " next_wakeup " << time_point_to_str(next_wakeup)
                      << " diff " << duration_to_str(next_wakeup - now1)
                      << " is this task ready?" << '\n';

            if (now1 <= next_wakeup) {
                // Keep sleeping
                std::cout << __func__ << " no, keep sleeping" << '\n';
                if (cv.wait_until(lock, next_wakeup, [this] {
                            //
                            // Wakeup reasons:
                            //
                            // - The timer is stopping
                            // - A new task has been added, need to re-consider next_wakeup
                            // - It's time to start the expected task
                            // - A spurious wakeup occurred
                            //
                            if (!m_running)
                                return false;

                            if (m_pq.empty())
                                return false;

                            const auto now = std::chrono::steady_clock::now();
                            if (m_pq.top().m_start_time <= now)
                                return true;

                            return false;
                        })) {
                    std::cout << __func__ << " ok, some task may be ready" << '\n';
                } else {
                    std::cout << __func__ << " stopping or empty queue, size " << m_pq.size() << '\n';
                }
            }

            if (m_pq.empty()) {
                // Someone removed all tasks while we slept
                continue;
            }

            // Let's do the job
            const auto now2 = std::chrono::steady_clock::now();
            Task task = m_pq.top();
            std::cout << __func__ << " ok is '" << task.m_name << "' ready?"
                      << " now2 " << time_point_to_str(now2)
                      << " start_time " << time_point_to_str(task.m_start_time)
                      << '\n';
            if (task.m_start_time <= now2) {
                std::cout << __func__ << " Yes, call callback and remove it" << '\n';
                task.m_func(task.m_user_data);
                m_pq.pop();
                if (task.m_type == Type::Periodic) {
                    std::cout << __func__ << " The task '" << task.m_name << "' is periodic, re-schedule" << '\n';
                    // FIXME Maybe also take into account previous
                    // start_time of a task
                    const std::chrono::steady_clock::time_point start_time =
                        std::chrono::steady_clock::now() + task.m_timeout;
                    task.m_start_time = start_time;
                    m_pq.push(std::move(task));
                }
            } else {
                std::cout << __func__ << " no" << '\n';
            }
        }
    });
}

void TaskScheduler::stop()
{
    if (!m_running)
        return;

    // Wake up and stop the worker thread
    cv.notify_one();
    m_running = false;
    m_thread.join();
}
