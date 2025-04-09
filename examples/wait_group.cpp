#include "coro/coro.hpp"

using namespace coro;

wait_group wg{0};

task<> wait()
{
    co_await wg.wait();
    log::info("wait finish");
}

task<> done()
{
    utils::sleep(2);
    wg.done();
    log::info("done...");
    co_return;
}

int main(int argc, char const* argv[])
{
    /* code */
    scheduler::init();
    wg.add(3);

    submit_to_scheduler(wait());
    submit_to_scheduler(done());
    submit_to_scheduler(done());
    submit_to_scheduler(done());

    scheduler::loop();

    return 0;
}
