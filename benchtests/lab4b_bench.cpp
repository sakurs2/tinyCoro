#include <algorithm>
#include <latch>
#include <thread>

#include "bench_helper.hpp"
#include "benchmark/benchmark.h"
#include "coro/coro.hpp"

using namespace coro;

static const int thread_num = std::thread::hardware_concurrency();

template<typename latch_type>
void latch_bench(const int loop_num);

/*************************************************************
 *                 threadpool_stl_future                     *
 *************************************************************/

void countdown_tp(std::latch& lt, const int loop_num)
{
    loop_add;
    lt.count_down();
}

void wait_tp(std::latch& lt, const int loop_num)
{
    lt.wait();
    loop_add;
}

static void threadpool_stl_latch(benchmark::State& state)
{
    for (auto _ : state)
    {
        const int loop_num = state.range(0);

        const int countdown_num = thread_num / 2;
        const int wait_num      = thread_num - countdown_num;

        thread_pool pool;
        std::latch  lt(countdown_num);

        for (int i = 0; i < countdown_num; i++)
        {
            pool.submit_task([&]() { countdown_tp(lt, loop_num); });
        }
        for (int i = 0; i < wait_num; i++)
        {
            pool.submit_task([&]() { wait_tp(lt, loop_num); });
        }
        pool.start();
        pool.join();
    }
}

CORO_BENCHMARK3(threadpool_stl_latch, 100, 100000, 100000000);

/*************************************************************
 *                    coro_stl_latch                         *
 *************************************************************/

task<> countdown(std::latch& lt, const int loop_num)
{
    loop_add;
    lt.count_down();
    co_return;
}

task<> wait(std::latch& lt, const int loop_num)
{
    lt.wait();
    loop_add;
    co_return;
}

static void coro_stl_latch(benchmark::State& state)
{
    for (auto _ : state)
    {
        const int loop_num = state.range(0);
        latch_bench<std::latch>(loop_num);
    }
}

CORO_BENCHMARK3(coro_stl_latch, 100, 100000, 100000000);

/*************************************************************
 *                       coro_latch                          *
 *************************************************************/

task<> countdown(latch& lt, const int loop_num)
{
    loop_add;
    lt.count_down();
    co_return;
}

task<> wait(latch& lt, const int loop_num)
{
    co_await lt.wait();
    loop_add;
    co_return;
}

static void coro_latch(benchmark::State& state)
{
    for (auto _ : state)
    {
        const int loop_num = state.range(0);
        latch_bench<coro::latch>(loop_num);
    }
}

CORO_BENCHMARK3(coro_latch, 100, 100000, 100000000);

BENCHMARK_MAIN();

template<typename latch_type>
void latch_bench(const int loop_num)
{
    const int countdown_num = thread_num / 2;
    const int wait_num      = thread_num - countdown_num;

    scheduler::init();

    latch_type lt(countdown_num);

    for (int i = 0; i < countdown_num; i++)
    {
        submit_to_scheduler(countdown(lt, loop_num));
    }

    for (int i = 0; i < wait_num; i++)
    {
        submit_to_scheduler(wait(lt, loop_num));
    }

    scheduler::loop();
}