#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <queue>
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

class ConditionVarNotifyOneTest : public ::testing::TestWithParam<int>
{
protected:
    void SetUp() override { m_para.state = false; }

    void TearDown() override {}

public:
    struct test_paras
    {
        bool               state;
        mutex              mtx;
        condition_variable cv;
        std::vector<int>   vec;
    };

protected:
    test_paras m_para;
};

class ConditionVarNotifyAllTest : public ::testing::TestWithParam<int>
{
protected:
    void SetUp() override { m_para.id = 0; }

    void TearDown() override {}

public:
    struct test_paras
    {
        int                id;
        mutex              mtx;
        condition_variable cv;
        std::vector<int>   vec;
    };

protected:
    test_paras m_para;
};

class ConditionVarProducerConsumerTest : public ::testing::TestWithParam<std::tuple<int, int, int, int>>
{
protected:
    void SetUp() override { m_para.id = 0; }

    void TearDown() override {}

public:
    struct test_paras
    {
        int                id;
        std::vector<int>   vec;
        std::queue<int>    que;
        condition_variable consumer_cv;
        condition_variable producer_cv;
        mutex              mtx;
        int                stop_flag;
    };

protected:
    test_paras m_para;
};

task<> notify_one(ConditionVarNotifyOneTest::test_paras& para, int id, int loop_num)
{
    while (loop_num > 0)
    {
        auto lock = co_await para.mtx.lock_guard();
        co_await para.cv.wait(para.mtx, [&]() { return para.state == bool(id % 2); });
        para.state = !para.state;
        para.vec.push_back(id);
        loop_num -= 1;
        para.cv.notify_one();
    }
}

task<> notify_all(ConditionVarNotifyAllTest::test_paras& para, int id)
{
    auto lock = co_await para.mtx.lock_guard();
    co_await para.cv.wait(para.mtx, [&]() { return para.id == id; });
    para.vec.push_back(id);
    para.id += 1;
    para.cv.notify_all();
}

task<> producer(ConditionVarProducerConsumerTest::test_paras& para, int capacity, size_t num_per_producer)
{
    auto lock = co_await para.mtx.lock_guard();
    while (num_per_producer > 0)
    {
        co_await para.producer_cv.wait(para.mtx, [&]() { return para.que.size() < capacity; });

        int loop_cnt = std::min(num_per_producer, capacity - para.que.size());
        for (int i = 0; i < loop_cnt; i++)
        {
            para.que.push(para.id);
            para.id++;
            num_per_producer--;
        }
        para.consumer_cv.notify_one();
    }
    para.stop_flag -= 1;
}

task<> consumer(ConditionVarProducerConsumerTest::test_paras& para)
{
    auto lock = co_await para.mtx.lock_guard();
    while (para.stop_flag > 0 || !para.que.empty())
    {
        co_await para.consumer_cv.wait(para.mtx, [&]() { return !para.que.empty() || para.stop_flag <= 0; });
        int loop_cnt = para.que.size();
        for (int i = 0; i < loop_cnt; i++)
        {
            para.vec.push_back(para.que.front());
            para.que.pop();
        }
        para.producer_cv.notify_one();
    }
    para.consumer_cv.notify_all();
}

/*************************************************************
 *                          tests                            *
 *************************************************************/

TEST_P(ConditionVarNotifyOneTest, ConditionVarNotifyOne)
{
    int loop_num = GetParam();

    scheduler::init();

    submit_to_scheduler(notify_one(m_para, 0, loop_num));
    submit_to_scheduler(notify_one(m_para, 1, loop_num));

    scheduler::loop();

    ASSERT_EQ(m_para.vec.size(), 2 * loop_num);
    for (int i = 0; i < 2 * loop_num; i++)
    {
        ASSERT_EQ(m_para.vec[i], i % 2);
    }
}

INSTANTIATE_TEST_SUITE_P(ConditionVarNotifyOneTests, ConditionVarNotifyOneTest, ::testing::Values(100, 1000, 10000));

TEST_P(ConditionVarNotifyAllTest, ConditionVarNotifyAll)
{
    int task_num = GetParam();

    scheduler::init();
    for (int i = 0; i < task_num; i++)
    {
        submit_to_scheduler(notify_all(m_para, i));
    }

    scheduler::loop();

    ASSERT_EQ(m_para.vec.size(), task_num);
    for (int i = 0; i < task_num; i++)
    {
        ASSERT_EQ(m_para.vec[i], i);
    }
}

INSTANTIATE_TEST_SUITE_P(
    ConditionVarNotifyAllTests, ConditionVarNotifyAllTest, ::testing::Values(2, 10, 100, 1000, 10000));
// config::kMaxTestTaskNum is too big, just ignore

TEST_P(ConditionVarProducerConsumerTest, ConditionVarProducerConsumer)
{
    int producer_num, consumer_num, capacity, num_per_producer;
    std::tie(producer_num, consumer_num, capacity, num_per_producer) = GetParam();

    m_para.stop_flag = producer_num;

    scheduler::init();

    for (int i = 0; i < producer_num; i++)
    {
        submit_to_scheduler(producer(m_para, capacity, num_per_producer));
    }

    for (int i = 0; i < consumer_num; i++)
    {
        submit_to_scheduler(consumer(m_para));
    }

    scheduler::loop();

    ASSERT_EQ(m_para.vec.size(), num_per_producer * producer_num);
    for (int i = 0; i < num_per_producer * producer_num; i++)
    {
        ASSERT_EQ(m_para.vec[i], i);
    }
}

INSTANTIATE_TEST_SUITE_P(
    ConditionVarProducerConsumerTests,
    ConditionVarProducerConsumerTest,
    ::testing::Values(
        std::tuple(1, 1, 1, 1),
        std::tuple(1, 1, 1, 100),
        std::tuple(1, 1, 1, 10000),
        std::tuple(100, 100, 1, 1),
        std::tuple(100, 100, 1, 100),
        std::tuple(100, 100, 1, 1000),
        std::tuple(1000, 1000, 1, 10),
        std::tuple(1000, 1000, 1, 100),
        std::tuple(1, 1, 100, 1),
        std::tuple(1, 1, 100, 100),
        std::tuple(1, 1, 100, 10000),
        std::tuple(100, 100, 100, 1),
        std::tuple(100, 100, 100, 100),
        std::tuple(100, 100, 100, 10000),
        std::tuple(1000, 1000, 100, 100),
        std::tuple(1000, 1000, 100, 10000)));
