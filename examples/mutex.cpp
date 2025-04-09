#include "coro/coro.hpp"

using namespace coro;

#define TASK_NUM 100

mutex mtx;
int   data = 0;

task<> add_task(int i)
{
    co_await mtx.lock();
    log::info("task {} fetch lock", i);
    for (int i = 0; i < 10000; i++)
    {
        data += 1;
    }
    mtx.unlock();
    co_return;
}

int main(int argc, char const* argv[])
{
    /* code */
    scheduler::init();

    for (int i = 0; i < TASK_NUM; i++)
    {
        submit_to_scheduler(add_task(i));
    }

    scheduler::loop();
    assert(data == 1000000);
    return 0;
}
