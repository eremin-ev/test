/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 */

#include "lib/task-scheduler.h"

#include <iostream>

namespace {

static void task_callback(const void *user_data)
{
    std::cout << __func__ << " user_data " << user_data << '\n';
}

static int test_task_scheduler()
{
    TaskScheduler ts;

    ts.add("test-task", TaskScheduler::Type::Periodic, 2*sec, task_callback, reinterpret_cast<const void *>(0x1));
    ts.add("other-task-2", TaskScheduler::Type::Oneshot, 4*sec, task_callback, reinterpret_cast<const void *>(0x2));

    ts.start();

    std::cout << __func__ << " started timer " << &ts << " is running " << ts.running() << '\n';

    const auto period = std::chrono::milliseconds(7000);
    std::this_thread::sleep_for(period);

    std::cout << __func__ << " stopping timer " << &ts << " is running " << ts.running() << '\n';

    ts.stop();

    std::cout << __func__ << " stopped timer " << &ts << " is running " << ts.running() << '\n';

    return 0;
}

} // anonymous namespace

int main()
{
    int errors = 0;

    errors += test_task_scheduler();

    std::cout << "errors " << errors << '\n';

    return 0;
}
