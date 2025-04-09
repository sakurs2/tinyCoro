#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <tuple>
#include <vector>

#include "coro/coro.hpp"
#include "gtest/gtest.h"

using namespace coro;

/*************************************************************
 *                       pre-definition                      *
 *************************************************************/

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

class MutexTest : public ::testing::TestWithParam<std::tuple<int, int>>
{
protected:
    void SetUp() override { m_id = 0; }

    void TearDown() override {}

    int              m_id;
    mutex            m_mtx;
    std::vector<int> m_vec;
};

class MutexTrylockTest : public ::testing::Test
{
protected:
    void SetUp() override { m_id = 0; }

    void TearDown() override {}

    int              m_id;
    mutex            m_mtx;
    std::vector<int> m_vec;
};

class MutexHybridEventTest : public MutexTest
{
protected:
    int     m_setid{0};
    event<> m_ev;
};

class MutexHybridLatchTest : public ::testing::TestWithParam<std::tuple<int, int, int>>
{
protected:
    void SetUp() override { m_id = 0; }

    void TearDown() override {}

    int              m_id;
    mutex            m_mtx;
    std::vector<int> m_countdown_vec;
    std::vector<int> m_wait_vec;
};

class MutexHybridWaitgroupTest : public ::testing::TestWithParam<std::tuple<int, int, int>>
{
protected:
    void SetUp() override { m_id = 0; }

    void TearDown() override {}

    int              m_id;
    mutex            m_mtx;
    std::vector<int> m_done_vec;
    std::vector<int> m_wait_vec;
    wait_group       m_wg;
};

task<> lock_func(mutex& mtx, std::vector<int>& vec, int& id)
{
    auto guard = co_await mtx.lock_guard();
    vec.push_back(id);
    ++id;
}

task<> trylock_func(mutex& mtx, std::vector<int>& vec, int& id)
{
    while (!mtx.try_lock()) {}
    vec.push_back(id);
    ++id;
    mtx.unlock();
    co_return;
}

task<> set_func_hybrid_mutex(event<>& ev, int& setid, int& id)
{
    auto guard = event_guard(ev);
    utils::msleep(100);
    ++id;
    setid = id;
    co_return;
}

task<> wait_func_hybrid_mutex(event<>& ev, mutex& mtx, std::vector<int>& vec, int& id)
{
    co_await ev.wait();

    auto p = co_await mtx.lock_guard();
    ++id;
    vec.push_back(id);
}

task<> countdown_func_hybrid_mutex(latch& lt, mutex& mtx, std::vector<int>& vec, int& id)
{
    auto guard     = latch_guard{lt};
    auto mtx_guard = co_await mtx.lock_guard();
    vec.push_back((id++));
    co_return;
}

task<> wait_func_hybrid_mutex(latch& lt, mutex& mtx, std::vector<int>& vec, int& id)
{
    co_await lt.wait();
    auto mtx_guard = co_await mtx.lock_guard();
    vec.push_back((id++));
}

task<> done_func_hybrid_mutex(wait_group& wg, mutex& mtx, std::vector<int>& vec, int& id)
{
    {
        auto mtx_guard = co_await mtx.lock_guard();
        vec.push_back((id++));
    }
    wg.done();
    co_return;
}

task<> wait_func_hybrid_mutex(wait_group& wg, mutex& mtx, std::vector<int>& vec, int& id)
{
    co_await wg.wait();
    auto mtx_guard = co_await mtx.lock_guard();
    vec.push_back((id++));
}

/*************************************************************
 *                          tests                            *
 *************************************************************/

TEST_P(MutexTest, MultiFetchLock)
{
    int thread_num, func_num;
    std::tie(thread_num, func_num) = GetParam();

    scheduler::init(thread_num);

    for (int i = 0; i < func_num; i++)
    {
        submit_to_scheduler(lock_func(m_mtx, m_vec, m_id));
    }

    scheduler::loop();

    ASSERT_EQ(m_vec.size(), func_num);
    ASSERT_EQ(m_id, func_num);

    std::sort(m_vec.begin(), m_vec.end());
    for (int i = 0; i < func_num; i++)
    {
        ASSERT_EQ(m_vec[i], i);
    }
}

INSTANTIATE_TEST_SUITE_P(
    MutexTests,
    MutexTest,
    ::testing::Values(
        std::make_tuple(1, 1),
        std::make_tuple(1, 100),
        std::make_tuple(1, 10000),
        std::make_tuple(0, 1),
        std::make_tuple(0, 100),
        std::make_tuple(0, 10000),
        std::make_tuple(0, config::kMaxTestTaskNum)));

TEST_F(MutexTrylockTest, MultiTryLock)
{
    const int thread_num = std::thread::hardware_concurrency();
    const int func_num   = thread_num;

    scheduler::init(thread_num);

    for (int i = 0; i < func_num; i++)
    {
        submit_to_scheduler(trylock_func(m_mtx, m_vec, m_id));
    }

    scheduler::loop();

    ASSERT_EQ(m_vec.size(), func_num);
    ASSERT_EQ(m_id, func_num);

    std::sort(m_vec.begin(), m_vec.end());
    for (int i = 0; i < func_num; i++)
    {
        ASSERT_EQ(m_vec[i], i);
    }
}

TEST_P(MutexHybridEventTest, MutexHybridEvent)
{
    int thread_num, wait_num;
    std::tie(thread_num, wait_num) = GetParam();

    scheduler::init(thread_num);

    for (int i = 0; i < wait_num; i++)
    {
        submit_to_scheduler(wait_func_hybrid_mutex(m_ev, m_mtx, m_vec, m_id));
    }

    submit_to_scheduler(set_func_hybrid_mutex(m_ev, m_setid, m_id));

    scheduler::loop();

    ASSERT_EQ(m_setid, 1);
    ASSERT_EQ(m_id, wait_num + 1);
    ASSERT_EQ(m_vec.size(), wait_num);

    std::sort(m_vec.begin(), m_vec.end());
    for (int i = 0; i < wait_num; i++)
    {
        ASSERT_EQ(m_vec[i], i + 2);
    }
}

INSTANTIATE_TEST_SUITE_P(
    MutexHybridEventTests,
    MutexHybridEventTest,
    ::testing::Values(
        std::make_tuple(1, 1),
        std::make_tuple(1, 100),
        std::make_tuple(1, 10000),
        std::make_tuple(0, 1),
        std::make_tuple(0, 100),
        std::make_tuple(0, 10000),
        std::make_tuple(0, config::kMaxTestTaskNum)));

TEST_P(MutexHybridLatchTest, MutexHybridLatch)
{
    int thread_num, countdown_num, wait_num;
    std::tie(thread_num, countdown_num, wait_num) = GetParam();

    scheduler::init(thread_num);

    latch lt(countdown_num);

    for (int i = 0; i < wait_num; i++)
    {
        submit_to_scheduler(wait_func_hybrid_mutex(lt, m_mtx, m_wait_vec, m_id));
    }

    for (int i = 0; i < countdown_num; i++)
    {
        submit_to_scheduler(countdown_func_hybrid_mutex(lt, m_mtx, m_countdown_vec, m_id));
    }

    scheduler::loop();

    ASSERT_EQ(m_countdown_vec.size(), countdown_num);
    ASSERT_EQ(m_wait_vec.size(), wait_num);

    std::sort(m_countdown_vec.begin(), m_countdown_vec.end());
    std::sort(m_wait_vec.begin(), m_wait_vec.end());

    ASSERT_LT(*m_countdown_vec.rbegin(), *m_wait_vec.begin());

    for (int i = 0; i < countdown_num; i++)
    {
        ASSERT_EQ(m_countdown_vec[i], i);
    }

    for (int i = 0; i < wait_num; i++)
    {
        ASSERT_EQ(m_wait_vec[i], i + countdown_num);
    }
}

INSTANTIATE_TEST_SUITE_P(
    MutexHybridLatchTests,
    MutexHybridLatchTest,
    ::testing::Values(
        std::make_tuple(1, 1, 1),
        std::make_tuple(1, 1, 100),
        std::make_tuple(1, 1, 10000),
        std::make_tuple(1, 100, 1),
        std::make_tuple(1, 100, 100),
        std::make_tuple(1, 100, 10000),
        std::make_tuple(0, 1, 1),
        std::make_tuple(0, 1, 100),
        std::make_tuple(0, 1, 10000),
        std::make_tuple(0, 100, 1),
        std::make_tuple(0, 100, 100),
        std::make_tuple(0, 100, 10000),
        std::make_tuple(0, 100, config::kMaxTestTaskNum)));

TEST_P(MutexHybridWaitgroupTest, MutexHybridWaitgroup)
{
    int thread_num, done_num, wait_num;
    std::tie(thread_num, done_num, wait_num) = GetParam();

    scheduler::init(thread_num);

    for (int i = 0; i < wait_num; i++)
    {
        submit_to_scheduler(wait_func_hybrid_mutex(m_wg, m_mtx, m_wait_vec, m_id));
    }

    for (int i = 0; i < done_num; i++)
    {
        m_wg.add(1);
        submit_to_scheduler(done_func_hybrid_mutex(m_wg, m_mtx, m_done_vec, m_id));
    }

    scheduler::loop();

    ASSERT_EQ(m_done_vec.size(), done_num);
    ASSERT_EQ(m_wait_vec.size(), wait_num);

    std::sort(m_done_vec.begin(), m_done_vec.end());
    std::sort(m_wait_vec.begin(), m_wait_vec.end());

    ASSERT_LT(*m_done_vec.rbegin(), *m_wait_vec.begin());

    for (int i = 0; i < done_num; i++)
    {
        ASSERT_EQ(m_done_vec[i], i);
    }

    for (int i = 0; i < wait_num; i++)
    {
        ASSERT_EQ(m_wait_vec[i], i + done_num);
    }
}

INSTANTIATE_TEST_SUITE_P(
    MutexHybridWaitgroupTests,
    MutexHybridWaitgroupTest,
    ::testing::Values(
        std::make_tuple(1, 1, 1),
        std::make_tuple(1, 1, 100),
        std::make_tuple(1, 1, 10000),
        std::make_tuple(1, 100, 1),
        std::make_tuple(1, 100, 100),
        std::make_tuple(1, 100, 10000),
        std::make_tuple(0, 1, 1),
        std::make_tuple(0, 1, 100),
        std::make_tuple(0, 1, 10000),
        std::make_tuple(0, 100, 1),
        std::make_tuple(0, 100, 100),
        std::make_tuple(0, 100, 10000),
        std::make_tuple(0, 100, config::kMaxTestTaskNum)));
