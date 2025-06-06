#include "coro/coro.hpp"

using namespace coro;

task<int> square(int i)
{
    co_return i* i;
}

task<> parallel_compute()
{
    std::vector<task<int>> vec;
    for (int i = 1; i <= 5; i++)
    {
        vec.push_back(square(i));
    }
    auto answer = co_await parallel::parallel_func(
        vec,
        parallel::make_parallel_reduce_func(
            [](std::vector<int>& vec)
            {
                int sum = 0;
                for (auto it : vec)
                {
                    sum += it;
                }
                return sum;
            }));
    log::info("final answer: {}", answer);
}

int main(int argc, char const* argv[])
{
    /* code */
    scheduler::init();
    submit_to_scheduler(parallel_compute());
    scheduler::loop();
    return 0;
}
