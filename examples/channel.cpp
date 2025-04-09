#include "coro/coro.hpp"

using namespace coro;

channel<int> ch;

task<> producer(int i)
{
    for (int i = 0; i < 10; i++)
    {
        co_await ch.send(i);
    }

    ch.close();
    co_return;
}

task<> consumer(int i)
{
    while (true)
    {
        auto data = co_await ch.recv();
        if (data)
        {
            log::info("consumer {} receive data: {}", i, *data);
        }
        else
        {
            break;
        }
    }
}

int main(int argc, char const* argv[])
{
    /* code */
    scheduler::init();

    submit_to_scheduler(producer(0));
    submit_to_scheduler(consumer(1));

    scheduler::loop();
    return 0;
}
