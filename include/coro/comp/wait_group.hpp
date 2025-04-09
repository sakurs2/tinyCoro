/**
 * @file wait_group.hpp
 * @author JiahuiWang
 * @brief lab4c
 * @version 1.1
 * @date 2025-03-24
 *
 * @copyright Copyright (c) 2025
 *
 */
#pragma once

#include <atomic>
#include <coroutine>

#include "coro/detail/types.hpp"

namespace coro
{
class context;
using detail::awaiter_ptr;

class wait_group
{
public:
    struct awaiter
    {
        awaiter(context& ctx, wait_group& wg) noexcept : m_ctx(ctx), m_wg(wg) {}

        constexpr auto await_ready() noexcept -> bool { return false; }

        auto await_suspend(std::coroutine_handle<> handle) noexcept -> bool;

        auto await_resume() noexcept -> void;

        auto resume() noexcept -> void;

        context&                m_ctx;
        wait_group&             m_wg;
        awaiter*                m_next{nullptr};
        std::coroutine_handle<> m_await_coro{nullptr};
    };

    explicit wait_group(int count = 0) noexcept : m_count(count) {}

    auto add(int count) noexcept -> void;

    auto done() noexcept -> void;

    auto wait() noexcept -> awaiter;

private:
    friend awaiter;
    std::atomic<int32_t>     m_count;
    std::atomic<awaiter_ptr> m_state;
};

}; // namespace coro
