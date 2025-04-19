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

class EventTest : public ::testing::TestWithParam<std::tuple<int, int>>
{
protected:
    void SetUp() override
    {
        m_id  = 0;
        m_set = 0;
    }

    void TearDown() override {}

    int              m_set;
    std::vector<int> m_wait_vec;
    event<>          m_ev;
    std::atomic<int> m_id;
};

class EventValueTest : public ::testing::TestWithParam<std::tuple<int, int>>
{
protected:
    void SetUp() override {}

    void TearDown() override {}

    std::vector<int> m_wait_vec;
    event<int>       m_ev;
};

task<> set_func(event<>& ev, std::atomic<int>& id, int* p)
{
    auto guard = event_guard(ev);
    utils::msleep(100);
    *p = id.fetch_add(1, std::memory_order_acq_rel) + 1;
    co_return;
}

task<> wait_func(event<>& ev, std::atomic<int>& id, int* p)
{
    co_await ev.wait();
    *p = id.fetch_add(1, std::memory_order_acq_rel) + 1;
}

task<> set_value_func(event<int>& ev, int value)
{
    utils::msleep(100);
    ev.set(value);
    co_return;
}

task<> wait_value_func(event<int>& ev, int* p)
{
    auto val = co_await ev.wait();
    *p       = val;
}

/*************************************************************
 *                          tests                            *
 *************************************************************/

TEST_P(EventTest, SetAndWait)
{
    int thread_num, wait_num;
    std::tie(thread_num, wait_num) = GetParam();

    scheduler::init(thread_num);

    m_wait_vec = std::vector<int>(wait_num, 0);

    for (int i = 0; i < wait_num; i++)
    {
        submit_to_scheduler(wait_func(m_ev, m_id, &(m_wait_vec[i])));
    }

    submit_to_scheduler(set_func(m_ev, m_id, &m_set));

    scheduler::loop();

    std::sort(m_wait_vec.begin(), m_wait_vec.end());
    ASSERT_EQ(m_set, 1);
    for (int i = 0; i < wait_num; i++)
    {
        ASSERT_EQ(m_wait_vec[i], i + 2);
    }
}

INSTANTIATE_TEST_SUITE_P(
    EventTests,
    EventTest,
    ::testing::Values(
        std::make_tuple(1, 1),
        std::make_tuple(1, 100),
        std::make_tuple(1, 10000),
        std::make_tuple(0, 1),
        std::make_tuple(0, 100),
        std::make_tuple(0, 10000),
        std::make_tuple(0, config::kMaxTestTaskNum)));

TEST_P(EventValueTest, SetValueAndWait)
{
    srand(static_cast<unsigned int>(std::time(0)));
    int randnum = rand();

    int thread_num, wait_num;
    std::tie(thread_num, wait_num) = GetParam();

    scheduler::init(thread_num);

    m_wait_vec = std::vector<int>(wait_num, 0);

    for (int i = 0; i < wait_num; i++)
    {
        submit_to_scheduler(wait_value_func(m_ev, &(m_wait_vec[i])));
    }

    submit_to_scheduler(set_value_func(m_ev, randnum));

    scheduler::loop();

    std::sort(m_wait_vec.begin(), m_wait_vec.end());

    for (int i = 0; i < wait_num; i++)
    {
        ASSERT_EQ(m_wait_vec[i], randnum);
    }
}

INSTANTIATE_TEST_SUITE_P(
    EventValueTests,
    EventValueTest,
    ::testing::Values(
        std::make_tuple(1, 1),
        std::make_tuple(1, 100),
        std::make_tuple(1, 10000),
        std::make_tuple(0, 1),
        std::make_tuple(0, 100),
        std::make_tuple(0, 10000),
        std::make_tuple(0, config::kMaxTestTaskNum)));
