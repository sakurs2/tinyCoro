#include <algorithm>
#include <cstdlib>
#include <cstring>
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

class WhenallTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        m_id        = 0;
        m_waiter_id = 0;
    }

    void TearDown() override {}

    int              m_waiter_id;
    std::atomic<int> m_id;
    std::vector<int> m_waitee_vec;
};

class WhenallNestCallTest : public ::testing::TestWithParam<int>
{
protected:
    void SetUp() override { memset(m_mark, 0, sizeof(m_mark)); }

    void TearDown() override {}

    bool m_mark[16384];
};

class WhenallHybridMutexTest : public ::testing::TestWithParam<int>
{
protected:
    void SetUp() override {}

    void TearDown() override {}

    mutex            m_mtx;
    std::vector<int> m_vec;
};

task<> waitee(int& set_id, std::atomic<int>& id)
{
    auto val = id.fetch_add(1, std::memory_order_acq_rel);
    set_id   = val;
    co_return;
}

task<> waiter_cnt1(std::vector<int>& vec, int& waiter_id, std::atomic<int>& id)
{
    co_await when_all(waitee(vec[0], id));
    auto val  = id.fetch_add(1, std::memory_order_acq_rel);
    waiter_id = val;
    co_return;
}

task<> waiter_cnt4(std::vector<int>& vec, int& waiter_id, std::atomic<int>& id)
{
    co_await when_all(waitee(vec[0], id), waitee(vec[1], id), waitee(vec[2], id), waitee(vec[3], id));
    auto val  = id.fetch_add(1, std::memory_order_acq_rel);
    waiter_id = val;
    co_return;
}

task<> waiter_cnt10(std::vector<int>& vec, int& waiter_id, std::atomic<int>& id)
{
    co_await when_all(
        waitee(vec[0], id),
        waitee(vec[1], id),
        waitee(vec[2], id),
        waitee(vec[3], id),
        waitee(vec[4], id),
        waitee(vec[5], id),
        waitee(vec[6], id),
        waitee(vec[7], id),
        waitee(vec[8], id),
        waitee(vec[9], id));
    auto val  = id.fetch_add(1, std::memory_order_acq_rel);
    waiter_id = val;
    co_return;
}

task<int> waitee_value(int i)
{
    co_return i;
}

task<> waiter_value_cnt4(std::vector<int>& vec)
{
    auto results = co_await when_all(waitee_value(1), waitee_value(2), waitee_value(3), waitee_value(4));
    for (auto& it : results)
    {
        vec.push_back(it);
    }
}

task<> waiter_value_cnt10(std::vector<int>& vec)
{
    auto results = co_await when_all(
        waitee_value(1),
        waitee_value(2),
        waitee_value(3),
        waitee_value(4),
        waitee_value(5),
        waitee_value(6),
        waitee_value(7),
        waitee_value(8),
        waitee_value(9),
        waitee_value(10));
    for (auto& it : results)
    {
        vec.push_back(it);
    }
}

task<> waiter_nest_call(bool* mark, int id, int cur_depth, const int depth)
{
    if (cur_depth < depth)
    {
        co_await when_all(
            waiter_nest_call(mark, 2 * id, cur_depth + 1, depth),
            waiter_nest_call(mark, 2 * id + 1, cur_depth + 1, depth));
    }
    mark[id] = true;
    co_return;
}

task<> empty_func()
{
    co_return;
}

task<> whenall_hybrid_mutex(mutex& mtx, std::vector<int>& vec, int i)
{
    auto guard = co_await mtx.lock_guard();
    co_await when_all(empty_func(), empty_func(), empty_func(), empty_func());
    vec.push_back(i);
}

/*************************************************************
 *                          tests                            *
 *************************************************************/

TEST_F(WhenallTest, WhenallWaitTaskCnt1)
{
    const int wait_num = 1;

    m_waitee_vec = std::vector<int>(wait_num, 0);
    scheduler::init();

    submit_to_scheduler(waiter_cnt1(m_waitee_vec, m_waiter_id, m_id));

    scheduler::loop();

    ASSERT_EQ(m_id, wait_num + 1);
    ASSERT_EQ(m_waiter_id, m_id - 1);
    ASSERT_EQ(m_waitee_vec.size(), m_id - 1);
    std::sort(m_waitee_vec.begin(), m_waitee_vec.end());
    for (int i = 0; i < m_id - 1; i++)
    {
        ASSERT_EQ(m_waitee_vec[i], i);
    }
}

TEST_F(WhenallTest, WhenallWaitTaskCnt4)
{
    const int wait_num = 4;

    m_waitee_vec = std::vector<int>(wait_num, 0);
    scheduler::init();

    submit_to_scheduler(waiter_cnt4(m_waitee_vec, m_waiter_id, m_id));

    scheduler::loop();

    ASSERT_EQ(m_id, wait_num + 1);
    ASSERT_EQ(m_waiter_id, m_id - 1);
    ASSERT_EQ(m_waitee_vec.size(), m_id - 1);
    std::sort(m_waitee_vec.begin(), m_waitee_vec.end());
    for (int i = 0; i < m_id - 1; i++)
    {
        ASSERT_EQ(m_waitee_vec[i], i);
    }
}

TEST_F(WhenallTest, WhenallWaitTaskCnt10)
{
    const int wait_num = 10;

    m_waitee_vec = std::vector<int>(wait_num, 0);
    scheduler::init();

    submit_to_scheduler(waiter_cnt10(m_waitee_vec, m_waiter_id, m_id));

    scheduler::loop();

    ASSERT_EQ(m_id, wait_num + 1);
    ASSERT_EQ(m_waiter_id, m_id - 1);
    ASSERT_EQ(m_waitee_vec.size(), m_id - 1);
    std::sort(m_waitee_vec.begin(), m_waitee_vec.end());
    for (int i = 0; i < m_id - 1; i++)
    {
        ASSERT_EQ(m_waitee_vec[i], i);
    }
}

TEST_F(WhenallTest, WhenallValueWaitTaskCnt4)
{
    const int wait_num = 4;

    scheduler::init();

    submit_to_scheduler(waiter_value_cnt4(m_waitee_vec));

    scheduler::loop();

    ASSERT_EQ(m_waitee_vec.size(), wait_num);
    std::sort(m_waitee_vec.begin(), m_waitee_vec.end());
    for (int i = 0; i < wait_num; i++)
    {
        ASSERT_EQ(m_waitee_vec[i], i + 1);
    }
}

TEST_F(WhenallTest, WhenallValueWaitTaskCnt10)
{
    const int wait_num = 10;

    scheduler::init();

    submit_to_scheduler(waiter_value_cnt10(m_waitee_vec));

    scheduler::loop();

    ASSERT_EQ(m_waitee_vec.size(), wait_num);
    std::sort(m_waitee_vec.begin(), m_waitee_vec.end());
    for (int i = 0; i < wait_num; i++)
    {
        ASSERT_EQ(m_waitee_vec[i], i + 1);
    }
}

TEST_P(WhenallNestCallTest, WhenallNestCall)
{
    const int depth     = GetParam();
    const int total_num = (1 << depth) - 1;

    scheduler::init();

    submit_to_scheduler(waiter_nest_call(m_mark, 1, 1, depth));

    scheduler::loop();

    for (int i = 1; i <= total_num; i++)
    {
        ASSERT_EQ(m_mark[i], true);
    }
}

INSTANTIATE_TEST_SUITE_P(WhenallNestCallTests, WhenallNestCallTest, ::testing::Values(1, 4, 8, 10, 13));

TEST_P(WhenallHybridMutexTest, WhenallHybridMutex)
{
    int task_num = GetParam();

    scheduler::init();

    for (int i = 0; i < task_num; i++)
    {
        submit_to_scheduler(whenall_hybrid_mutex(m_mtx, m_vec, i));
    }

    scheduler::loop();

    ASSERT_EQ(m_vec.size(), task_num);
    std::sort(m_vec.begin(), m_vec.end());
    for (int i = 0; i < task_num; i++)
    {
        ASSERT_EQ(m_vec[i], i);
    }
}

INSTANTIATE_TEST_SUITE_P(WhenallHybridMutexTests, WhenallHybridMutexTest, ::testing::Values(10, 100, 1000, 10000));
