#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <queue>
#include <string>
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

class ChannelTest : public ::testing::TestWithParam<std::tuple<int, int, int>>
{
protected:
    void SetUp() override { m_para.id = 0; }

    void TearDown() override {}

public:
    struct test_paras
    {
        int              id;
        channel<int>     ch;
        std::vector<int> vec;
        mutex            mtx;
        wait_group       wg;
    };

protected:
    test_paras m_para;
};

class BufferChannelTest : public ::testing::TestWithParam<std::tuple<int, int, int>>
{
protected:
    void SetUp() override { m_para.id = 0; }

    void TearDown() override {}

public:
    struct test_paras
    {
        int                id;
        channel<int, 1024> ch;
        std::vector<int>   vec;
        mutex              mtx;
        wait_group         wg;
    };

protected:
    test_paras m_para;
};

class ChannelStringTest : public ::testing::Test
{
protected:
    void SetUp() override { m_para.id = 0; }

    void TearDown() override {}

public:
    struct test_paras
    {
        int                      id;
        channel<std::string>     ch;
        std::vector<std::string> vec;
        mutex                    mtx;
        wait_group               wg;
    };

    inline static const char* test_str = "tinycoro";

protected:
    test_paras m_para;
};

task<> producer(ChannelTest::test_paras& para, int id, const int num_per_producer)
{
    for (int i = 0; i < num_per_producer; i++)
    {
        co_await para.ch.send(id * num_per_producer + i);
    }
    para.wg.done();
}

task<> consumer(ChannelTest::test_paras& para)
{
    std::vector<int> vec;
    while (true)
    {
        auto data = co_await para.ch.recv();
        if (data)
        {
            vec.push_back(*data);
        }
        else
        {
            break;
        }
    }

    co_await para.mtx.lock();
    para.vec.insert(para.vec.end(), vec.begin(), vec.end());
    para.mtx.unlock();
}

task<> close(ChannelTest::test_paras& para)
{
    co_await para.wg.wait();
    para.ch.close();
}

task<> producer(BufferChannelTest::test_paras& para, int id, const int num_per_producer)
{
    for (int i = 0; i < num_per_producer; i++)
    {
        co_await para.ch.send(id * num_per_producer + i);
    }
    para.wg.done();
}

task<> consumer(BufferChannelTest::test_paras& para)
{
    std::vector<int> vec;
    while (true)
    {
        auto data = co_await para.ch.recv();
        if (data)
        {
            vec.push_back(*data);
        }
        else
        {
            break;
        }
    }

    co_await para.mtx.lock();
    para.vec.insert(para.vec.end(), vec.begin(), vec.end());
    para.mtx.unlock();
}

task<> close(BufferChannelTest::test_paras& para)
{
    co_await para.wg.wait();
    para.ch.close();
}

task<> producer(ChannelStringTest::test_paras& para, int id, const int num_per_producer)
{
    for (int i = 0; i < num_per_producer; i++)
    {
        co_await para.ch.send(ChannelStringTest::test_str);
    }
    para.ch.close();
}

task<> consumer(ChannelStringTest::test_paras& para)
{
    while (true)
    {
        auto data = co_await para.ch.recv();
        if (data)
        {
            para.vec.push_back(std::move(*data));
        }
        else
        {
            break;
        }
    }
}
/*************************************************************
 *                          tests                            *
 *************************************************************/

TEST_P(ChannelTest, ChannelProducerConsumer)
{
    int producer_num, consumer_num, num_per_producer;
    std::tie(producer_num, consumer_num, num_per_producer) = GetParam();

    const int task_num = num_per_producer * producer_num;
    m_para.wg.add(producer_num);

    scheduler::init();

    submit_to_scheduler(close(m_para));

    for (int i = 0; i < producer_num; i++)
    {
        submit_to_scheduler(producer(m_para, i, num_per_producer));
    }

    for (int i = 0; i < consumer_num; i++)
    {
        submit_to_scheduler(consumer(m_para));
    }

    scheduler::loop();

    ASSERT_EQ(m_para.vec.size(), task_num);
    std::sort(m_para.vec.begin(), m_para.vec.end());
    for (int i = 0; i < task_num; i++)
    {
        ASSERT_EQ(m_para.vec[i], i);
    }
}

INSTANTIATE_TEST_SUITE_P(
    ChannelTests,
    ChannelTest,
    ::testing::Values(
        std::tuple(1, 1, 100),
        std::tuple(1, 1, 1000),
        std::tuple(1, 1, 10000),
        std::tuple(1, 1, config::kMaxTestTaskNum),

        std::tuple(10, 10, 100),
        std::tuple(10, 10, 10000),

        std::tuple(100, 100, 100),
        std::tuple(100, 100, 1000),

        std::tuple(1000, 1000, 1),
        std::tuple(1000, 1000, 100),

        std::tuple(1, 1000, config::kMaxTestTaskNum),
        std::tuple(1000, 1, 100)));

TEST_P(BufferChannelTest, BufferChannelProducerConsumer)
{
    int producer_num, consumer_num, num_per_producer;
    std::tie(producer_num, consumer_num, num_per_producer) = GetParam();
    m_para.wg.add(producer_num);

    const int task_num = num_per_producer * producer_num;

    scheduler::init();

    submit_to_scheduler(close(m_para));

    for (int i = 0; i < producer_num; i++)
    {
        submit_to_scheduler(producer(m_para, i, num_per_producer));
    }

    for (int i = 0; i < consumer_num; i++)
    {
        submit_to_scheduler(consumer(m_para));
    }

    scheduler::loop();

    ASSERT_EQ(m_para.vec.size(), task_num);
    std::sort(m_para.vec.begin(), m_para.vec.end());
    for (int i = 0; i < task_num; i++)
    {
        ASSERT_EQ(m_para.vec[i], i);
    }
}

INSTANTIATE_TEST_SUITE_P(
    BufferChannelTests,
    BufferChannelTest,
    ::testing::Values(
        std::tuple(1, 1, 100),
        std::tuple(1, 1, 1000),
        std::tuple(1, 1, 10000),
        std::tuple(1, 1, config::kMaxTestTaskNum),

        std::tuple(10, 10, 100),
        std::tuple(10, 10, 10000),

        std::tuple(100, 100, 100),
        std::tuple(100, 100, 1000),

        std::tuple(1000, 1000, 1),
        std::tuple(1000, 1000, 100),

        std::tuple(1, 1000, config::kMaxTestTaskNum),
        std::tuple(1000, 1, 100)));

TEST_F(ChannelStringTest, StringChannelProducerConsumer)
{
    const int         num_per_producer = 100;
    const std::string str{ChannelStringTest::test_str};

    scheduler::init();

    submit_to_scheduler(producer(m_para, 0, num_per_producer));
    submit_to_scheduler(consumer(m_para));

    scheduler::loop();

    ASSERT_EQ(m_para.vec.size(), num_per_producer);
    for (int i = 0; i < num_per_producer; i++)
    {
        ASSERT_EQ(m_para.vec[i], str);
    }
}