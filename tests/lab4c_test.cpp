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

class WaitgroupTest : public ::testing::TestWithParam<std::tuple<int, int, int>>
{
protected:
    void SetUp() override { m_id = 0; }

    void TearDown() override {}

    wait_group       m_wg;
    std::atomic<int> m_id;
    std::vector<int> m_done_vec;
    std::vector<int> m_wait_vec;
};

task<> done_func(wait_group& wg, std::atomic<int>& id, int* data)
{
    *data = id.fetch_add(1, std::memory_order_acq_rel);
    wg.done();
    co_return;
}

task<> wait_func(wait_group& wg, std::atomic<int>& id, int* data)
{
    co_await wg.wait();
    *data = id.fetch_add(1, std::memory_order_acq_rel);
}

/*************************************************************
 *                          tests                            *
 *************************************************************/

TEST_P(WaitgroupTest, DoneAndWait)
{
    int thread_num, done_num, wait_num;
    std::tie(thread_num, done_num, wait_num) = GetParam();

    scheduler::init(thread_num);

    m_done_vec = std::vector(done_num, 0);
    m_wait_vec = std::vector(wait_num, 0);

    for (int i = 0; i < wait_num; i++)
    {
        submit_to_scheduler(wait_func(m_wg, m_id, &(m_wait_vec[i])));
    }

    for (int i = 0; i < done_num; i++)
    {
        m_wg.add(1);
        submit_to_scheduler(done_func(m_wg, m_id, &(m_done_vec[i])));
    }

    scheduler::loop();

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
    WaitgroupTests,
    WaitgroupTest,
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
