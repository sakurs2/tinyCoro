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

class LatchTest : public ::testing::TestWithParam<std::tuple<int, int, int>>
{
protected:
    void SetUp() override { m_id = 0; }

    void TearDown() override {}

    std::atomic<int> m_id;
    std::vector<int> m_countdown_vec;
    std::vector<int> m_wait_vec;
};

task<> countdown_func(latch& lt, std::atomic<int>& id, int* data)
{
    auto guard = latch_guard{lt};
    *data      = id.fetch_add(1, std::memory_order_acq_rel);
    co_return;
}

task<> wait_func(latch& lt, std::atomic<int>& id, int* data)
{
    co_await lt.wait();
    *data = id.fetch_add(1, std::memory_order_acq_rel);
}

/*************************************************************
 *                          tests                            *
 *************************************************************/

TEST_P(LatchTest, CountdownAndWait)
{
    int thread_num, countdown_num, wait_num;
    std::tie(thread_num, countdown_num, wait_num) = GetParam();

    scheduler::init(thread_num);

    latch lt(countdown_num);

    m_countdown_vec = std::vector(countdown_num, 0);
    m_wait_vec      = std::vector(wait_num, 0);

    for (int i = 0; i < wait_num; i++)
    {
        submit_to_scheduler(wait_func(lt, m_id, &(m_wait_vec[i])));
    }

    for (int i = 0; i < countdown_num; i++)
    {
        submit_to_scheduler(countdown_func(lt, m_id, &(m_countdown_vec[i])));
    }

    scheduler::loop();

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
    LatchTests,
    LatchTest,
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
