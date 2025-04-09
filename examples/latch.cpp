#include "coro/coro.hpp"

using namespace coro;

#define TASK_NUM 5

latch l(TASK_NUM - 1);

task<> set_task(int i)
{
    log::info("set task ready to sleep");
    utils::sleep(2);
    l.count_down();
    co_return;
}

task<> wait_task(int i)
{
    co_await l.wait();
    log::info("task {} wake up by latch", i);
    co_return;
}

int main(int argc, char const* argv[])
{
    /* code */
    scheduler::init();

    submit_to_scheduler(wait_task(TASK_NUM - 1));
    for (int i = 0; i < TASK_NUM - 1; i++)
    {
        submit_to_scheduler(set_task(i));
    }

    scheduler::loop();
    return 0;
}
