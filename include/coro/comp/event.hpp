/**
 * @file event.hpp
 * @author JiahuiWang
 * @brief lab4a
 * @version 1.1
 * @date 2025-03-24
 *
 * @copyright Copyright (c) 2025
 *
 */
#pragma once
#include <atomic>
#include <coroutine>

#include "coro/attribute.hpp"
#include "coro/concepts/awaitable.hpp"
#include "coro/context.hpp"
#include "coro/detail/container.hpp"
#include "coro/detail/types.hpp"

namespace coro
{
class context;

namespace detail
{
class event_base
{
public:
    struct awaiter_base
    {
        awaiter_base(context& ctx, event_base& e) noexcept : m_ctx(ctx), m_ev(e) {}

        inline auto next() noexcept -> awaiter_base* { return m_next; }

        auto await_ready() noexcept -> bool;

        auto await_suspend(std::coroutine_handle<> handle) noexcept -> bool;

        auto await_resume() noexcept -> void;

        context&                m_ctx;
        event_base&             m_ev;
        awaiter_base*           m_next{nullptr};
        std::coroutine_handle<> m_await_coro{nullptr};
    };

    event_base(bool initial_set = false) noexcept : m_state((initial_set) ? this : nullptr) {}
    ~event_base() noexcept = default;

    event_base(const event_base&)            = delete;
    event_base(event_base&&)                 = delete;
    event_base& operator=(const event_base&) = delete;
    event_base& operator=(event_base&&)      = delete;

    inline auto is_set() const noexcept -> bool { return m_state.load(std::memory_order_acquire) == this; }

    auto set_state() noexcept -> void;

    auto resume_all_awaiter(awaiter_ptr waiter) noexcept -> void;

    auto register_awaiter(awaiter_base* waiter) noexcept -> bool;

private:
    std::atomic<awaiter_ptr> m_state{nullptr};
};
}; // namespace detail

template<typename return_type = void>
class event : public detail::event_base, public detail::container<return_type>
{
public:
    using event_base::event_base;
    struct [[CORO_AWAIT_HINT]] awaiter : public detail::event_base::awaiter_base
    {
        using awaiter_base::awaiter_base;
        auto await_resume() noexcept -> decltype(auto)
        {
            detail::event_base::awaiter_base::await_resume();
            return static_cast<event&>(m_ev).result();
        }
    };

    [[CORO_AWAIT_HINT]] awaiter wait() noexcept { return awaiter(local_context(), *this); }

    template<typename value_type>
    auto set(value_type&& value) noexcept -> void
    {
        this->return_value(std::forward<value_type>(value));
        set_state();
    }
};

template<>
class event<void> : public detail::event_base
{
public:
    using event_base::event_base;
    struct [[CORO_AWAIT_HINT]] awaiter : public detail::event_base::awaiter_base
    {
        using awaiter_base::awaiter_base;
    };

    [[CORO_AWAIT_HINT]] awaiter wait() noexcept { return awaiter(local_context(), *this); }

    auto set() noexcept -> void { set_state(); }
};

class event_guard
{
    using guard_type = event<>;

public:
    event_guard(guard_type& ev) noexcept : m_ev(ev) {}
    ~event_guard() noexcept { m_ev.set(); }

private:
    guard_type& m_ev;
};

}; // namespace coro
