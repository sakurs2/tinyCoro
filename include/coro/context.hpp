#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <thread>

#include "config.h"
#include "coro/engine.hpp"
#include "coro/meta_info.hpp"
#include "coro/task.hpp"

namespace coro
{
using config::ctx_id;
using std::atomic;
using std::jthread;
using std::make_unique;
using std::memory_order_acq_rel;
using std::memory_order_acquire;
using std::memory_order_relaxed;
using std::stop_token;
using std::unique_ptr;

using detail::ginfo;
using detail::linfo;

using engine = detail::engine;

class scheduler;

/**
 * @brief each context own one engine, it's the core part of tinycoro,
 * which can process computation task and io task
 *
 */
class context
{
    using stop_cb = std::function<void()>;

public:
    context() noexcept;
    ~context() noexcept                = default;
    context(const context&)            = delete;
    context(context&&)                 = delete;
    context& operator=(const context&) = delete;
    context& operator=(context&&)      = delete;

    /**
     * @brief work thread start running
     *
     */
    [[CORO_TEST_USED(lab2b)]] auto start() noexcept -> void;

    /**
     * @brief send stop signal to work thread
     *
     */
    [[CORO_TEST_USED(lab2b)]] auto notify_stop() noexcept -> void;

    /**
     * @brief wait work thread stop
     *
     */
    inline auto join() noexcept -> void { m_job->join(); }

    /**
     * @brief Set the stop cb object
     *
     * @param cb
     */
    auto set_stop_cb(stop_cb cb) noexcept -> void;

    inline auto submit_task(task<void>&& task) noexcept -> void
    {
        auto handle = task.handle();
        task.detach();
        this->submit_task(handle);
    }

    inline auto submit_task(task<void>& task) noexcept -> void { submit_task(task.handle()); }

    /**
     * @brief submit one task handle to context
     *
     * @param handle
     */
    [[CORO_TEST_USED(lab2b)]] inline auto submit_task(std::coroutine_handle<> handle) noexcept -> void
    {
        m_engine.submit_task(handle);
    }

    /**
     * @brief get context unique id
     *
     * @return ctx_id
     */
    inline auto get_ctx_id() noexcept -> ctx_id { return m_id; }

    /**
     * @brief add reference count of context
     *
     * @param register_cnt
     */
    [[CORO_TEST_USED(lab2b)]] inline auto register_wait(int register_cnt = 1) noexcept -> void CORO_INLINE
    {
        m_num_wait_task.fetch_add(size_t(register_cnt), memory_order_acq_rel);
    }

    /**
     * @brief reduce reference count of context
     *
     * @param register_cnt
     */
    [[CORO_TEST_USED(lab2b)]] inline auto unregister_wait(int register_cnt = 1) noexcept -> void CORO_INLINE
    {
        m_num_wait_task.fetch_sub(register_cnt, memory_order_acq_rel);
    }

    inline auto get_engine() noexcept -> engine& { return m_engine; }

    [[CORO_TEST_USED(lab2b)]] auto init() noexcept -> void;

    [[CORO_TEST_USED(lab2b)]] auto deinit() noexcept -> void;

    /**
     * @brief main logic of work thread
     *
     * @param token
     */
    [[CORO_TEST_USED(lab2b)]] auto run(stop_token token) noexcept -> void;

    auto process_work() noexcept -> void;

    inline auto poll_work() noexcept -> void { m_engine.poll_submit(); }

    /**
     * @brief return if reference count is zero
     *
     * @return true
     * @return false
     */
    [[CORO_TEST_USED(lab2b)]] inline auto empty_wait_task() noexcept -> bool CORO_INLINE
    {
        return m_num_wait_task.load(memory_order_acquire) == 0 && m_engine.empty_io();
    }

private:
    CORO_ALIGN engine   m_engine;
    unique_ptr<jthread> m_job;
    ctx_id              m_id;
    atomic<size_t>      m_num_wait_task{0};
    stop_cb             m_stop_cb;
};

inline context& local_context() noexcept
{
    return *linfo.ctx;
}

inline void submit_to_context(task<void>&& task) noexcept
{
    local_context().submit_task(std::move(task));
}

inline void submit_to_context(task<void>& task) noexcept
{
    local_context().submit_task(task.handle());
}

inline void submit_to_context(std::coroutine_handle<> handle) noexcept
{
    local_context().submit_task(handle);
}

}; // namespace coro
