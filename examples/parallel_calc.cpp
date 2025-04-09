#include <vector>

#include "coro/coro.hpp"

using namespace coro;

#define TASK_NUM 5

std::vector<int> vec{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};

task<> calc(int id, int lef, int rig, std::vector<int>& v)
{
    int sum = 0;
    for (int i = lef; i < rig; i++)
    {
        sum += v[i];
    }
    log::info("task {} calc result: {}", id, sum);
    co_return;
}

int main(int argc, char const* argv[])
{
    /* code */
    scheduler::init();

    for (int i = 0; i < TASK_NUM; i++)
    {
        submit_to_scheduler(calc(i, i * 3, (i + 1) * 3, vec));
    }

    scheduler::loop();
    return 0;
}
