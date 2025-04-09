#include "coro/coro.hpp"

using namespace coro;

#define TASK_NUM 5

event<> ev;

task<> set_task()
{
    event_guard guard(ev);
    log::info("set task ready to sleep");
    utils::sleep(3);
    log::info("ready to set event");
    co_return;
}

task<> wait_task(int i)
{
    co_await ev.wait();
    log::info("task {} wake up by event", i);
    co_return;
}

int main(int argc, char const* argv[])
{
    /* code */
    scheduler::init();

    for (int i = 0; i < TASK_NUM - 1; i++)
    {
        submit_to_scheduler(wait_task(i));
    }
    submit_to_scheduler(set_task());

    scheduler::loop();
    return 0;
}
