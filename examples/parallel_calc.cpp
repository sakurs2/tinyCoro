#include <vector>

#include "coro/coro.hpp"

using namespace coro;

#define CONTEXTNUM 5

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
    context ctx[CONTEXTNUM];
    for (int i = 0; i < CONTEXTNUM; i++)
    {
        ctx[i].submit_task(calc(i, i * 3, (i + 1) * 3, vec));
        log::info("context {} submit task", i);
    }

    for (int i = 0; i < CONTEXTNUM; i++)
    {
        ctx[i].start();
        log::info("context {} start task", i);
    }

    for (int i = 0; i < CONTEXTNUM; i++)
    {
        ctx[i].stop();
        log::info("context {} stop finish", i);
    }
    return 0;
}
