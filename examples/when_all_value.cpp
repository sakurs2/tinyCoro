#include "coro/coro.hpp"

using namespace coro;

task<int> sub_func(int i)
{
    log::info("sub func {} running...", i);
    co_return i;
}

task<> func(int i)
{
    log::info("ready to wait all sub task");
    auto data = co_await when_all(sub_func(i + 1), sub_func(i + 2), sub_func(i + 3));
    for (auto& it : data)
    {
        log::info("receive data: {}", it);
    }
    log::info("wait all sub task finish");
    co_return;
}

int main(int argc, char const* argv[])
{
    /* code */
    scheduler::init();

    submit_to_scheduler(func(0));

    scheduler::loop();
    return 0;
}
