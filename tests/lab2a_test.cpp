#include <algorithm>
#include <thread>
#include <tuple>
#include <vector>

#include "coro/engine.hpp"
#include "coro/io/io_info.hpp"
#include "coro/task.hpp"
#include "coro/utils.hpp"
#include "gtest/gtest.h"

using namespace coro;

/*************************************************************
 *                       pre-definition                      *
 *************************************************************/

using ::coro::io::detail::cb_type;
using ::coro::io::detail::io_info;
using ::coro::io::detail::io_type;

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

class EngineTest : public ::testing::Test
{
protected:
    void SetUp() override { m_engine.init(); }

    void TearDown() override { m_engine.deinit(); }

    detail::engine   m_engine;
    std::vector<int> m_vec; // use vector to verify test result
};

class EngineNopIOTest : public ::testing::TestWithParam<int>
{
protected:
    void SetUp() override { m_engine.init(); }

    void TearDown() override { m_engine.deinit(); }

    detail::engine       m_engine;
    std::vector<int>     m_vec;
    std::vector<io_info> m_infos;
};

class EngineMultiThreadTaskTest : public EngineNopIOTest
{
};

class EngineMixTaskNopIOTest : public ::testing::TestWithParam<std::tuple<int, int>>
{
protected:
    void SetUp() override { m_engine.init(); }

    void TearDown() override { m_engine.deinit(); }

    detail::engine       m_engine;
    std::vector<int>     m_vec;
    std::vector<io_info> m_infos;
};

task<> func(std::vector<int>& vec, int val)
{
    vec.push_back(val);
    co_return;
}

void io_cb(io_info* info, int res)
{
    auto num = reinterpret_cast<int*>(info->data);
    *num     = res;
}

void io_cb_resume(io_info* info, int res)
{
    auto num = reinterpret_cast<int*>(info->data);
    *num     = res;
    detail::local_engine().submit_task(info->handle);
}

struct test_noop_awaiter
{
    test_noop_awaiter(detail::engine& engine, io_info& info, int* data) noexcept : m_engine(engine), m_info(info)
    {
        m_sqe       = m_engine.get_free_urs();
        m_info.data = reinterpret_cast<uintptr_t>(data);
        m_info.cb   = io_cb_resume;

        io_uring_prep_nop(m_sqe);
        io_uring_sqe_set_data(m_sqe, &m_info);

        m_engine.add_io_submit();
    }

    constexpr auto await_ready() noexcept -> bool { return false; }

    auto await_suspend(std::coroutine_handle<> handle) noexcept -> void { m_info.handle = handle; }

    constexpr auto await_resume() noexcept -> void {}

    detail::engine&     m_engine;
    io_info&            m_info;
    coro::uring::ursptr m_sqe;
};

task<> noop_io_task(detail::engine& engine, io_info& info, int* data)
{
    co_await test_noop_awaiter(engine, info, data);
    co_return;
}

/*************************************************************
 *                          tests                            *
 *************************************************************/

// test whether the initial state is correct
TEST_F(EngineTest, InitStateCase)
{
    ASSERT_FALSE(m_engine.ready());
    ASSERT_TRUE(m_engine.empty_io());
    ASSERT_EQ(m_engine.num_task_schedule(), 0);
    ASSERT_EQ(m_engine.get_id(), detail::local_engine().get_id());
}

// test submit detach task but exec by user
TEST_F(EngineTest, ExecOneDetachTaskByUser)
{
    auto task = func(m_vec, 1);
    m_engine.submit_task(task.handle());
    task.detach();

    ASSERT_TRUE(m_engine.ready());
    ASSERT_EQ(m_engine.num_task_schedule(), 1);
    auto handle = m_engine.schedule();
    ASSERT_FALSE(m_engine.ready());
    ASSERT_EQ(m_engine.num_task_schedule(), 0);
    handle.resume();
    ASSERT_EQ(m_vec.size(), 1);
    ASSERT_EQ(m_vec[0], 1);
    handle.destroy();
}

// test submit many detach tasks but exec by user
TEST_F(EngineTest, ExecMultiDetachTaskByUser)
{
    const int task_num = 100;
    for (int i = 0; i < task_num; i++)
    {
        auto task = func(m_vec, i);
        m_engine.submit_task(task.handle());
        task.detach();
    }

    ASSERT_TRUE(m_engine.ready());
    ASSERT_EQ(m_engine.num_task_schedule(), task_num);
    while (m_engine.ready())
    {
        auto handle = m_engine.schedule();
        handle.resume();
        handle.destroy();
    }
    ASSERT_EQ(m_engine.num_task_schedule(), 0);
    ASSERT_EQ(m_vec.size(), task_num);

    std::sort(m_vec.begin(), m_vec.end());
    for (int i = 0; i < task_num; i++)
    {
        ASSERT_EQ(m_vec[i], i);
    }
}

// test submit detach task but exec by engine, the engine
// should destroy task handler, otherwise memory leaks
TEST_F(EngineTest, ExecOneDetachTaskByEngine)
{
    auto task = func(m_vec, 1);
    m_engine.submit_task(task.handle());
    task.detach();

    ASSERT_TRUE(m_engine.ready());
    ASSERT_EQ(m_engine.num_task_schedule(), 1);

    m_engine.exec_one_task();

    ASSERT_FALSE(m_engine.ready());
    ASSERT_EQ(m_engine.num_task_schedule(), 0);

    ASSERT_EQ(m_vec.size(), 1);
    ASSERT_EQ(m_vec[0], 1);
}

// test submit many detach task but exec by engine, the engine
// should destroy task handler, otherwise memory leaks
TEST_F(EngineTest, ExecMultiDetachTaskByEngine)
{
    const int task_num = 100;
    for (int i = 0; i < task_num; i++)
    {
        auto task   = func(m_vec, i);
        auto handle = task.handle();
        task.detach();
        m_engine.submit_task(handle);
    }

    ASSERT_TRUE(m_engine.ready());
    ASSERT_EQ(m_engine.num_task_schedule(), task_num);
    while (m_engine.ready())
    {
        m_engine.exec_one_task();
    }
    ASSERT_EQ(m_engine.num_task_schedule(), 0);
    ASSERT_EQ(m_vec.size(), task_num);

    std::sort(m_vec.begin(), m_vec.end());
    for (int i = 0; i < task_num; i++)
    {
        ASSERT_EQ(m_vec[i], i);
    }
}

// test submit task but exec by engine, the engine
// shouldn't destroy task handler, otherwise cause pointer double free.
TEST_F(EngineTest, ExecOneNoDetachTaskByEngine)
{
    auto task = func(m_vec, 1);
    m_engine.submit_task(task.handle());

    ASSERT_TRUE(m_engine.ready());
    ASSERT_EQ(m_engine.num_task_schedule(), 1);
    m_engine.exec_one_task();
    ASSERT_FALSE(m_engine.ready());
    ASSERT_EQ(m_engine.num_task_schedule(), 0);

    ASSERT_EQ(m_vec.size(), 1);
    ASSERT_EQ(m_vec[0], 1);
}

// TODO: Add more nopio tests for engine
// // test add nop-io before engine poll
// TEST_F(EngineTest, AddNopIOBeforePoll)
// {
//     io_info info;
//     m_vec.push_back(1);

//     auto io_thread = std::thread(
//         [&]()
//         {
//             info.data = reinterpret_cast<uintptr_t>(&m_vec[0]);
//             info.cb   = io_cb;
//             auto sqe  = m_engine.get_free_urs();
//             ASSERT_NE(sqe, nullptr);
//             io_uring_prep_nop(sqe);
//             io_uring_sqe_set_data(sqe, &info);
//             m_engine.add_io_submit();
//         });

//     auto poll_thread = std::thread(
//         [&]()
//         {
//             utils::msleep(100); // ensure io_thread finish first
//             do
//             {
//                 m_engine.poll_submit();
//             } while (!m_engine.empty_io());
//         });

//     io_thread.join();
//     poll_thread.join();

//     ASSERT_EQ(m_vec[0], 0);
// }

// // test add nop-io after engine poll
// TEST_F(EngineTest, AddNopIOAfterPoll)
// {
//     io_info info;
//     m_vec.push_back(1);

//     auto io_thread = std::thread(
//         [&]()
//         {
//             utils::msleep(100);
//             info.data = reinterpret_cast<uintptr_t>(&m_vec[0]);
//             info.cb   = io_cb;
//             auto sqe  = m_engine.get_free_urs();
//             ASSERT_NE(sqe, nullptr);
//             io_uring_prep_nop(sqe);
//             io_uring_sqe_set_data(sqe, &info);
//             m_engine.add_io_submit();
//         });

//     auto poll_thread = std::thread(
//         [&]()
//         {
//             do
//             {
//                 m_engine.poll_submit();
//             } while (!m_engine.empty_io());
//         });

//     io_thread.join();
//     poll_thread.join();

//     ASSERT_EQ(m_vec[0], 0);
// }

// test add batch nop-io
TEST_P(EngineNopIOTest, AddBatchNopIO)
{
    int task_num = GetParam();
    m_infos.resize(task_num);
    m_vec = std::vector<int>(task_num, 1);
    for (int i = 0; i < task_num; i++)
    {
        m_infos[i].data = reinterpret_cast<uintptr_t>(&m_vec[i]);
        m_infos[i].cb   = io_cb;
        auto sqe        = m_engine.get_free_urs();
        ASSERT_NE(sqe, nullptr);
        io_uring_prep_nop(sqe);
        io_uring_sqe_set_data(sqe, &m_infos[i]);
        m_engine.add_io_submit();
    }

    do
    {
        m_engine.poll_submit();
    } while (!m_engine.empty_io());

    for (int i = 0; i < task_num; i++)
    {
        ASSERT_EQ(m_vec[i], 0);
    }
}

INSTANTIATE_TEST_SUITE_P(EngineNopIOTests, EngineNopIOTest, ::testing::Values(1, 100, 10000));

// test add nop-io in loop
TEST_F(EngineTest, LoopAddNopIO)
{
    const int loop_num = 2 * config::kEntryLength;
    m_vec.push_back(0);
    for (int i = 0; i < loop_num; i++)
    {
        m_vec[0] = 1;
        io_info info;
        info.data = reinterpret_cast<uintptr_t>(&m_vec[0]);
        info.cb   = io_cb;

        auto sqe = m_engine.get_free_urs();
        ASSERT_NE(sqe, nullptr);
        io_uring_prep_nop(sqe);
        io_uring_sqe_set_data(sqe, &info);
        m_engine.add_io_submit();

        do
        {
            m_engine.poll_submit();
        } while (!m_engine.empty_io());
        ASSERT_EQ(m_vec[0], 0);
    }
}

// test submit task after engine poll
TEST_F(EngineTest, LastSubmitTaskToEngine)
{
    m_vec.push_back(0);
    auto task = func(m_vec, 0);

    auto t1 = std::thread(
        [&]()
        {
            m_engine.poll_submit();
            m_vec[0] = 1;
            ASSERT_TRUE(m_engine.ready());
            ASSERT_EQ(m_engine.num_task_schedule(), 1);
            ASSERT_TRUE(m_engine.empty_io());
        });
    auto t2 = std::thread(
        [&]()
        {
            utils::msleep(100);
            m_vec[0] = 2;
            m_engine.submit_task(task.handle());
        });
    t1.join();
    t2.join();

    ASSERT_EQ(m_vec[0], 1);
}

// test submit task before engine poll
TEST_F(EngineTest, FirstSubmitTaskToEngine)
{
    m_vec.push_back(0);
    auto task = func(m_vec, 0);

    auto t1 = std::thread(
        [&]()
        {
            utils::msleep(100);
            m_engine.poll_submit();
            m_vec[0] = 1;
            ASSERT_TRUE(m_engine.ready());
            ASSERT_EQ(m_engine.num_task_schedule(), 1);
            ASSERT_TRUE(m_engine.empty_io());
        });
    auto t2 = std::thread(
        [&]()
        {
            m_vec[0] = 2;
            m_engine.submit_task(task.handle());
        });
    t1.join();
    t2.join();

    ASSERT_EQ(m_vec[0], 1);
}

// test submit task but exec by user
TEST_F(EngineTest, SubmitTaskToEngineExecByUser)
{
    m_vec.push_back(0);
    auto task = func(m_vec, 2);

    auto t1 = std::thread(
        [&]()
        {
            m_engine.poll_submit();
            m_vec[0] = 1;
            ASSERT_TRUE(m_engine.ready());
            ASSERT_EQ(m_engine.num_task_schedule(), 1);
            ASSERT_TRUE(m_engine.empty_io());
        });
    auto t2 = std::thread(
        [&]()
        {
            utils::msleep(100);
            m_vec[0] = 2;
            m_engine.submit_task(task.handle());
        });
    t1.join();
    t2.join();

    task.handle().resume();

    ASSERT_EQ(m_vec.size(), 2);
    ASSERT_EQ(m_vec[0], 1);
    ASSERT_EQ(m_vec[1], 2);
}

// test submit task but exec by engine
TEST_F(EngineTest, SubmitTaskToEngineExecByEngine)
{
    m_vec.push_back(0);
    auto task = func(m_vec, 2);

    auto t1 = std::thread(
        [&]()
        {
            m_engine.poll_submit();
            m_vec[0] = 1;
            ASSERT_TRUE(m_engine.ready());
            ASSERT_EQ(m_engine.num_task_schedule(), 1);
            ASSERT_TRUE(m_engine.empty_io());
            m_engine.exec_one_task();
        });
    auto t2 = std::thread(
        [&]()
        {
            utils::msleep(100);
            m_vec[0] = 2;
            m_engine.submit_task(task.handle());
        });
    t1.join();
    t2.join();

    ASSERT_EQ(m_vec.size(), 2);
    ASSERT_EQ(m_vec[0], 1);
    ASSERT_EQ(m_vec[1], 2);
}

// test submit task by multi-thread
TEST_P(EngineMultiThreadTaskTest, MultiThreadAddTask)
{
    int thread_num = GetParam();

    auto t = std::thread(
        [&]()
        {
            int count = 0;
            while (count < thread_num)
            {
                m_engine.poll_submit();
                while (m_engine.ready())
                {
                    ++count;
                    m_engine.exec_one_task();
                }
            }
        });

    std::vector<std::thread> vec;
    for (int i = 0; i < thread_num; i++)
    {
        vec.push_back(std::thread(
            [&, i]()
            {
                auto task   = func(m_vec, i);
                auto handle = task.handle();
                task.detach();
                m_engine.submit_task(handle);
            }));
    }

    t.join();
    for (int i = 0; i < thread_num; i++)
    {
        vec[i].join();
    }

    std::sort(m_vec.begin(), m_vec.end());
    for (int i = 0; i < thread_num; i++)
    {
        EXPECT_EQ(m_vec[i], i);
    }
}

INSTANTIATE_TEST_SUITE_P(EngineMultiThreadTaskTests, EngineMultiThreadTaskTest, ::testing::Values(1, 10, 100));

// test submit task and nop-io
TEST_P(EngineMixTaskNopIOTest, MixTaskNopIO)
{
    int task_num, nopio_num;
    std::tie(task_num, nopio_num) = GetParam();
    m_vec.reserve(task_num + nopio_num + 1);
    m_vec.resize(nopio_num);
    m_infos.resize(nopio_num);

    auto task_thread = std::thread(
        [&]()
        {
            for (int i = 0; i < task_num; i++)
            {
                auto task   = func(m_vec, nopio_num + i);
                auto handle = task.handle();
                task.detach();
                m_engine.submit_task(handle);
                if ((i + 1) % 100 == 0)
                {
                    utils::msleep(10);
                }
            }
        });

    auto io_thread = std::thread(
        [&]()
        {
            for (int i = 0; i < nopio_num; i++)
            {
                // m_infos[i].data = reinterpret_cast<uintptr_t>(&m_vec[i]);
                // m_infos[i].cb   = io_cb;

                // auto sqe = m_engine.get_free_urs();
                // ASSERT_NE(sqe, nullptr);
                // io_uring_prep_nop(sqe);
                // io_uring_sqe_set_data(sqe, &m_infos[i]);

                // m_engine.add_io_submit();
                auto task   = noop_io_task(m_engine, m_infos[i], &(m_vec[i]));
                auto handle = task.handle();
                task.detach();
                m_engine.submit_task(handle);
                if ((i + 1) % 100 == 0)
                {
                    utils::msleep(10);
                }
            }

            // these will make poll_thread finish after io_thread
            auto task   = func(m_vec, task_num + nopio_num);
            auto handle = task.handle();
            task.detach();
            m_engine.submit_task(handle);
        });

    auto poll_thread = std::thread(
        [&]()
        {
            detail::linfo.egn = &m_engine; // Must set this

            int cnt = 0;
            // only all task been processed, poll thread will finish
            while (cnt < 2 * nopio_num + task_num + 1)
            {
                m_engine.poll_submit();
                while (m_engine.ready())
                {
                    m_engine.exec_one_task();
                    cnt++;
                }
            }
        });

    task_thread.join();
    io_thread.join();
    poll_thread.join();

    ASSERT_EQ(m_vec.size(), task_num + nopio_num + 1);
    std::sort(m_vec.begin(), m_vec.end());
    for (int i = 0; i < nopio_num; i++)
    {
        ASSERT_EQ(m_vec[i], 0);
    }
    for (int i = 0; i < task_num + 1; i++)
    {
        ASSERT_EQ(m_vec[i + nopio_num], i + nopio_num);
    }
}

INSTANTIATE_TEST_SUITE_P(
    EngineMixTaskNopIOTests,
    EngineMixTaskNopIOTest,
    ::testing::Values(
        std::make_tuple(1, 1), std::make_tuple(100, 100), std::make_tuple(1000, 1000), std::make_tuple(10000, 10000)));
