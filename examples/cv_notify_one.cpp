#include <queue>

#include "coro/coro.hpp"

using namespace coro;

cond_var        cv;
mutex           mtx;
std::queue<int> que;

task<> consumer()
{
    int  cnt{0};
    auto lock = co_await mtx.lock_guard();
    log::info("consumer hold lock");
    while (cnt < 100)
    {
        co_await cv.wait(mtx, [&]() { return !que.empty(); });
        while (!que.empty())
        {
            log::info("consumer fetch value: {}", que.front());
            que.pop();
            cnt++;
        }
        log::info("consumer cnt: {}", cnt);
        cv.notify_one();
    }
    log::info("consumer finish");
}

task<> producer()
{
    for (int i = 0; i < 10; i++)
    {
        auto lock = co_await mtx.lock_guard();
        log::info("producer hold lock");
        co_await cv.wait(mtx, [&]() { return que.size() < 10; });
        for (int j = 0; j < 10; j++)
        {
            que.push(i * 10 + j);
        }
        log::info("producer add value finish");
        cv.notify_one();
    }
}

int main(int argc, char const* argv[])
{
    /* code */
    scheduler::init();

    submit_to_scheduler(consumer());
    submit_to_scheduler(producer());

    scheduler::loop();
    return 0;
}
