#include "coro/coro.hpp"

using namespace coro;

#define TASK_NUM 100

cond_var cv;
mutex    mtx;
uint64_t data = 0;
int      id   = 0;

task<> run(int i)
{
    auto lock = co_await mtx.lock_guard();
    co_await cv.wait(mtx, [&]() { return i == id; });
    for (int i = 0; i < 100000; i++)
    {
        data++;
    }
    log::info("task {} run finish, result: {}", i, data);
    id++;
    cv.notify_all();
}

int main(int argc, char const* argv[])
{
    /* code */
    scheduler::init();

    for (int i = 0; i < TASK_NUM; i++)
    {
        submit_to_scheduler(run(i));
    }

    scheduler::loop();
    return 0;
}
