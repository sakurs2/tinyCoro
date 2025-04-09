#include "coro/coro.hpp"

using namespace coro;

channel<int, 5>  ch;
std::atomic<int> number;

task<> producer(int id)
{
    for (int i = 0; i < 4; i++)
    {
        co_await ch.send(id * 10 + i);
        log::info("producer {} send once", id);
    }

    if (number.fetch_sub(1, std::memory_order_acq_rel) == 1)
    {
        ch.close();
    }
    co_return;
}

task<> consumer(int id)
{
    while (true)
    {
        auto data = co_await ch.recv();
        if (data)
        {
            log::info("consumer {} receive data: {}", id, *data);
        }
        else
        {
            log::info("consumer {} receive close", id);
            break;
        }
    }
}

int main(int argc, char const* argv[])
{
    /* code */
    scheduler::init();

    number = 2;
    submit_to_scheduler(producer(0));
    submit_to_scheduler(producer(1));
    submit_to_scheduler(consumer(2));
    submit_to_scheduler(consumer(3));

    scheduler::loop();
    return 0;
}
