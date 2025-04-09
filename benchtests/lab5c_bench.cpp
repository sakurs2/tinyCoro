#include <algorithm>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <string>
#include <thread>

#include "bench_helper.hpp"
#include "benchmark/benchmark.h"
#include "coro/coro.hpp"

using namespace coro;

static const int thread_num = std::thread::hardware_concurrency();
static const int capacity   = 16;

template<typename return_type>
void channel_producer_tp(std::condition_variable& cv, std::mutex& mtx, std::queue<return_type>& que, int total_num)
{
    while (total_num > 0)
    {
        std::unique_lock<std::mutex> lck(mtx);
        cv.wait(lck, [&]() { return que.size() < capacity; });
        int cnt = capacity - que.size();
        if (total_num < cnt)
        {
            cnt = total_num;
        }
        for (int i = 0; i < cnt; i++)
        {
            if constexpr (std::is_same_v<return_type, std::string>)
            {
                que.push(std::string(bench_str));
            }
            else
            {
                que.push(return_type{});
            }
        }
        total_num -= cnt;
        cv.notify_all();
    }
}

template<typename return_type>
void channel_consumer_tp(std::condition_variable& cv, std::mutex& mtx, std::queue<return_type>& que, int total_num)
{
    return_type p;
    while (total_num > 0)
    {
        std::unique_lock<std::mutex> lck(mtx);
        cv.wait(lck, [&]() { return !que.empty(); });
        while (!que.empty())
        {
            benchmark::DoNotOptimize(p = std::move(que.front()));
            que.pop();
            total_num--;
        }
        cv.notify_all();
    }
}

static void threadpool_stl_channel_int(benchmark::State& state)
{
    for (auto _ : state)
    {
        const int               loop_num = state.range(0);
        thread_pool             pool;
        std::condition_variable cv;
        std::mutex              mtx;
        std::queue<int>         que;

        pool.submit_task([&]() { channel_producer_tp<int>(cv, mtx, que, loop_num * capacity); });
        pool.submit_task([&]() { channel_consumer_tp<int>(cv, mtx, que, loop_num * capacity); });
        pool.start();
        pool.join();
    }
}

CORO_BENCHMARK2(threadpool_stl_channel_int, 100, 10000);

static void threadpool_stl_channel_string(benchmark::State& state)
{
    for (auto _ : state)
    {
        const int               loop_num = state.range(0);
        thread_pool             pool;
        std::condition_variable cv;
        std::mutex              mtx;
        std::queue<std::string> que;

        pool.submit_task([&]() { channel_producer_tp<std::string>(cv, mtx, que, loop_num * capacity); });
        pool.submit_task([&]() { channel_consumer_tp<std::string>(cv, mtx, que, loop_num * capacity); });
        pool.start();
        pool.join();
    }
}

CORO_BENCHMARK2(threadpool_stl_channel_string, 100, 10000);

/*************************************************************
 *                         send int                          *
 *************************************************************/

template<typename channel_type, typename return_type>
void channel_bench(const int loop_num);

template<typename return_type>
task<> channel_producer(std::condition_variable& cv, std::mutex& mtx, std::queue<return_type>& que, int total_num)
{
    while (total_num > 0)
    {
        std::unique_lock<std::mutex> lck(mtx);
        cv.wait(lck, [&]() { return que.size() < capacity; });
        int cnt = capacity - que.size();
        if (total_num < cnt)
        {
            cnt = total_num;
        }
        for (int i = 0; i < cnt; i++)
        {
            if constexpr (std::is_same_v<return_type, std::string>)
            {
                que.push(std::string(bench_str));
            }
            else
            {
                que.push(return_type{});
            }
        }
        total_num -= cnt;
        cv.notify_all();
    }
    co_return;
}

template<typename return_type>
task<> channel_consumer(std::condition_variable& cv, std::mutex& mtx, std::queue<return_type>& que, int total_num)
{
    return_type p;
    while (total_num > 0)
    {
        std::unique_lock<std::mutex> lck(mtx);
        cv.wait(lck, [&]() { return !que.empty(); });
        while (!que.empty())
        {
            benchmark::DoNotOptimize(p = std::move(que.front()));
            que.pop();
            total_num--;
        }
        cv.notify_all();
    }
    co_return;
}

static void coro_stl_channel_int(benchmark::State& state)
{
    for (auto _ : state)
    {
        const int loop_num = state.range(0);
        channel_bench<std::condition_variable, int>(loop_num);
    }
}

CORO_BENCHMARK2(coro_stl_channel_int, 100, 10000);

static void coro_stl_channel_string(benchmark::State& state)
{
    for (auto _ : state)
    {
        const int loop_num = state.range(0);
        channel_bench<std::condition_variable, std::string>(loop_num);
    }
}

CORO_BENCHMARK2(coro_stl_channel_string, 100, 10000);

template<typename return_type>
task<> channel_producer(channel<return_type, capacity>& ch, int total_num)
{
    for (int i = 0; i < total_num; i++)
    {
        if constexpr (std::is_same_v<return_type, std::string>)
        {
            co_await ch.send(std::string(bench_str));
        }
        else
        {
            co_await ch.send(return_type{});
        }
    }
    ch.close();
}

template<typename return_type>
task<> channel_consumer(channel<return_type, capacity>& ch)
{
    while (true)
    {
        auto data = co_await ch.recv();
        if (!data)
        {
            break;
        }
    }
}

static void coro_channel_int(benchmark::State& state)
{
    for (auto _ : state)
    {
        const int loop_num = state.range(0);
        channel_bench<channel<int>, int>(loop_num);
    }
}

CORO_BENCHMARK2(coro_channel_int, 100, 10000);

static void coro_channel_string(benchmark::State& state)
{
    for (auto _ : state)
    {
        const int loop_num = state.range(0);
        channel_bench<channel<std::string>, std::string>(loop_num);
    }
}

CORO_BENCHMARK2(coro_channel_string, 100, 10000);

BENCHMARK_MAIN();

template<typename channel_type, typename return_type>
void channel_bench(const int loop_num)
{
    scheduler::init();

    if constexpr (std::is_same_v<channel_type, std::condition_variable>)
    {
        std::condition_variable cv;
        std::mutex              mtx;
        std::queue<return_type> que;

        submit_to_scheduler(channel_producer(cv, mtx, que, loop_num * capacity));
        submit_to_scheduler(channel_consumer(cv, mtx, que, loop_num * capacity));

        scheduler::loop();
    }
    else
    {
        channel<return_type, capacity> ch;
        submit_to_scheduler(channel_producer(ch, loop_num * capacity));
        submit_to_scheduler(channel_consumer(ch));

        scheduler::loop();
    }
}