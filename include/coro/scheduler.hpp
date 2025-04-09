#pragma once

#include <atomic>
#include <memory>
#include <thread>
#include <vector>

#include "config.h"
#include "coro/detail/atomic_helper.hpp"
#include "coro/dispatcher.hpp"

namespace coro
{

/**
 * @brief scheduler just control context to run and stop,
 * it also use dispatcher to decide which context can accept the task
 *
 */
class scheduler
{
    friend context;
    using stop_token_type = std::atomic<int>;
    using stop_flag_type  = std::vector<detail::atomic_ref_wrapper<int>>;

public:
    inline static auto init(size_t ctx_cnt = std::thread::hardware_concurrency()) noexcept -> void
    {
        if (ctx_cnt == 0)
        {
            ctx_cnt = std::thread::hardware_concurrency();
        }
        get_instance()->init_impl(ctx_cnt);
    }

    /**
     * @brief loop work, auto wait all context finish job
     *
     */
    inline static auto loop() noexcept -> void { get_instance()->loop_impl(); }

    static inline auto submit(task<void>&& task) noexcept -> void
    {
        auto handle = task.handle();
        task.detach();
        submit(handle);
    }

    static inline auto submit(task<void>& task) noexcept -> void { submit(task.handle()); }

    inline static auto submit(std::coroutine_handle<> handle) noexcept -> void
    {
        get_instance()->submit_task_impl(handle);
    }

private:
    static auto get_instance() noexcept -> scheduler*
    {
        static scheduler sc;
        return &sc;
    }

    auto init_impl(size_t ctx_cnt) noexcept -> void;

    auto start_impl() noexcept -> void;

    auto loop_impl() noexcept -> void;

    auto stop_impl() noexcept -> void;

    auto submit_task_impl(std::coroutine_handle<> handle) noexcept -> void;

private:
    size_t                                              m_ctx_cnt{0};
    detail::ctx_container                               m_ctxs;
    detail::dispatcher<coro::config::kDispatchStrategy> m_dispatcher;
    stop_flag_type                                      m_ctx_stop_flag;
    stop_token_type                                     m_stop_token;
};

inline void submit_to_scheduler(task<void>&& task) noexcept
{
    scheduler::submit(std::move(task));
}

inline void submit_to_scheduler(task<void>& task) noexcept
{
    scheduler::submit(task.handle());
}

inline void submit_to_scheduler(std::coroutine_handle<> handle) noexcept
{
    scheduler::submit(handle);
}

}; // namespace coro
