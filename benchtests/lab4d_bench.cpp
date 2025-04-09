#include <algorithm>
#include <mutex>
#include <thread>

#include "bench_helper.hpp"
#include "benchmark/benchmark.h"
#include "coro/coro.hpp"

using namespace coro;

static const int thread_num = std::thread::hardware_concurrency();

template<typename mutex_type>
void mutex_bench(const int loop_num);

/*************************************************************
 *                 threadpool_stl_mutex                      *
 *************************************************************/

static void add_tp(std::mutex& mtx, const int loop_num)
{
    mtx.lock();
    loop_add;
    mtx.unlock();
}

static void threadpool_stl_mutex(benchmark::State& state)
{
    for (auto _ : state)
    {
        const int loop_num = state.range(0);

        thread_pool pool;
        std::mutex  mtx;
        size_t      cnt = 0;
        for (int i = 0; i < thread_num; i++)
        {
            pool.submit_task([&]() { add_tp(mtx, loop_num); });
        }

        pool.start();
        pool.join();
    }
}

CORO_BENCHMARK3(threadpool_stl_mutex, 100, 100000, 100000000);

/*************************************************************
 *                    coro_stl_mutex                         *
 *************************************************************/

static task<> add(std::mutex& mtx, const int loop_num)
{
    mtx.lock();
    loop_add;
    mtx.unlock();
    co_return;
}

static void coro_stl_mutex(benchmark::State& state)
{
    for (auto _ : state)
    {
        const int loop_num = state.range(0);
        mutex_bench<std::mutex>(loop_num);
    }
}

CORO_BENCHMARK3(coro_stl_mutex, 100, 100000, 100000000);

/*************************************************************
 *                     coro_spinlock                         *
 *************************************************************/

static task<> add(detail::spinlock& mtx, const int loop_num)
{
    mtx.lock();
    loop_add;
    mtx.unlock();
    co_return;
}

static void coro_spinlock(benchmark::State& state)
{
    for (auto _ : state)
    {
        const int loop_num = state.range(0);
        mutex_bench<detail::spinlock>(loop_num);
    }
}

CORO_BENCHMARK3(coro_spinlock, 100, 100000, 100000000);

/*************************************************************
 *                       coro_mutex                          *
 *************************************************************/

static task<> add(mutex& mtx, const int loop_num)
{
    co_await mtx.lock();
    loop_add;
    mtx.unlock();
}

static void coro_mutex(benchmark::State& state)
{
    for (auto _ : state)
    {
        const int loop_num = state.range(0);
        mutex_bench<mutex>(loop_num);
    }
}

CORO_BENCHMARK3(coro_mutex, 100, 100000, 100000000);

BENCHMARK_MAIN();

template<typename mutex_type>
void mutex_bench(const int loop_num)
{
    scheduler::init();

    mutex_type mtx;

    for (int i = 0; i < thread_num; i++)
    {
        submit_to_scheduler(add(mtx, loop_num));
    }

    scheduler::loop();
}
