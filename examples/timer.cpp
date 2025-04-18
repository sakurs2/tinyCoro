#include "coro/coro.hpp"

using namespace coro;

using coro::time::timer;

task<> timer_func()
{
    log::info("timer begin");
    auto result = co_await timer{}.add_seconds(3).add_mseconds(500);
    log::info("timer end,  result: {}", result);
}

int main(int argc, char const* argv[])
{
    /* code */
    scheduler::init();
    submit_to_scheduler(timer_func());
    scheduler::loop();
    return 0;
}
