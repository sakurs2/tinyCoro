#pragma once

#include <array>
#include <atomic>
#include <coroutine>
#include <functional>
#include <queue>

#include "config.h"
#include "coro/atomic_que.hpp"
#include "coro/attribute.hpp"
#include "coro/meta_info.hpp"
#include "coro/uring_proxy.hpp"

namespace coro
{
class context;
};

namespace coro::detail
{
using std::array;
using std::atomic;
using std::coroutine_handle;
using std::queue;
using uring::urcptr;
using uring::uring_proxy;
using uring::ursptr;

inline engine& local_engine() noexcept;

template<typename T>
// multi producer and multi consumer queue
using mpmc_queue = AtomicQueue<T>;

#define wake_by_task(val) (((val)&engine::task_mask) > 0)
#define wake_by_io(val)   (((val)&engine::io_mask) > 0)
#define wake_by_cqe(val)  (((val)&engine::cqe_mask) > 0)

class engine
{
    friend class ::coro::context;

public:
    static constexpr uint64_t task_mask = (0xFFFFF00000000000);
    static constexpr uint64_t io_mask   = (0x00000FFFFF000000);
    static constexpr uint64_t cqe_mask  = (0x0000000000FFFFFF);

    static constexpr uint64_t task_flag = (((uint64_t)1) << 44);
    static constexpr uint64_t io_flag   = (((uint64_t)1) << 24);

    engine() noexcept : m_num_io_wait_submit(0), m_num_io_running(0)
    {
        m_id = ginfo.engine_id.fetch_add(1, std::memory_order_relaxed);
    }

    ~engine() noexcept = default;

    // forbidden to copy and move
    engine(const engine&)                    = delete;
    engine(engine&&)                         = delete;
    auto operator=(const engine&) -> engine& = delete;
    auto operator=(engine&&) -> engine&      = delete;

    [[CORO_TEST_USED(lab2a)]] auto init() noexcept -> void;

    [[CORO_TEST_USED(lab2a)]] auto deinit() noexcept -> void;

    /**
     * @brief return if engine has task to run
     *
     * @return true
     * @return false
     */
    [[CORO_TEST_USED(lab2a)]] inline auto ready() noexcept -> bool { return !m_task_queue.was_empty(); }

    /**
     * @brief get the free uring sqe entry
     *
     * @return ursptr
     */
    [[CORO_DISCARD_HINT]] inline auto get_free_urs() noexcept -> ursptr { return m_upxy.get_free_sqe(); }

    /**
     * @brief return number of task to run in engine
     *
     * @return size_t
     */
    [[CORO_TEST_USED(lab2a)]] inline auto num_task_schedule() noexcept -> size_t { return m_task_queue.was_size(); }

    /**
     * @brief fetch one task handle and engine should erase it from its queue
     *
     * @return coroutine_handle<>
     */
    [[CORO_TEST_USED(lab2a)]] [[CORO_DISCARD_HINT]] auto schedule() noexcept -> coroutine_handle<>;

    /**
     * @brief submit one task handle to engine
     *
     * @param handle
     */
    [[CORO_TEST_USED(lab2a)]] auto submit_task(coroutine_handle<> handle) noexcept -> void;

    /**
     * @brief this will call schedule() to fetch one task handle and run it
     *
     * @note this function will call clean() when handle is done
     */
    auto exec_one_task() noexcept -> void;

    /**
     * @brief handle one cqe entry
     *
     * @param cqe io_uring cqe entry
     */
    auto handle_cqe_entry(urcptr cqe) noexcept -> void;

    /**
     * @brief submit uring sqe and wait uring finish, then handle
     * cqe entry by call handle_cqe_entry
     *
     */
    [[CORO_TEST_USED(lab2a)]] auto poll_submit() noexcept -> void;

    /**
     * @brief write flag to eventfd to wake up thread blocked by read eventfd
     *
     * @param val
     */
    auto wake_up(uint64_t val = engine::task_flag) noexcept -> void;

    /**
     * @brief tell engine there has one io to be submitted, engine will record this
     *
     */
    [[CORO_TEST_USED(lab2a)]] inline auto add_io_submit() noexcept -> void
    {
        m_num_io_wait_submit += 1;

        // WARNING: don't need to wake_up
        // if don't wake up, the poll_submit may alaways blocked in
        // read evebtfd
        // wake_up(io_flag);
    }

    /**
     * @brief return if there has any io to be processed,
     * io include two types: submit_io, running_io
     *
     * @note submit_io: io to be submitted
     * @note running_io: io is submitted but it has't run finished
     *
     * @return true
     * @return false
     */
    [[CORO_TEST_USED(lab2a)]] inline auto empty_io() noexcept -> bool
    {
        return m_num_io_wait_submit == 0 && m_num_io_running == 0;
    }

    /**
     * @brief return engine unique id
     *
     * @return uint32_t
     */
    inline auto get_id() noexcept -> uint32_t { return m_id; }

    /**
     * @brief Get the uring object owned by engine, so the outside can use
     * some function of uring
     *
     * @return uring_proxy&
     */
    inline auto get_uring() noexcept -> uring_proxy& { return m_upxy; }

private:
    auto do_io_submit() noexcept -> void;

    auto exec_task(coroutine_handle<> handle) -> void;

    inline auto is_local_engine() noexcept -> bool { return get_id() == local_engine().get_id(); };

private:
    uint32_t    m_id;
    uring_proxy m_upxy;

    // store task handle
    mpmc_queue<coroutine_handle<>> m_task_queue;

    // used to fetch cqe entry
    array<urcptr, config::kQueCap> m_urc;

    // io_uring is recommended to use in only one thread,
    // so IO task fetch sqe won't cross-threading, don't need to use atomic
    // the number of io to be submitted
    size_t m_num_io_wait_submit{0};
    // the number of io running
    size_t m_num_io_running{0};
};

/**
 * @brief return local thread engine
 *
 * @return engine&
 */
inline engine& local_engine() noexcept
{
    return *linfo.egn;
}

}; // namespace coro::detail
