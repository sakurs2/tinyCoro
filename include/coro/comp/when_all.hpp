/**
 * @file when_all.hpp
 * @author JiahuiWang
 * @brief lab5a
 * @version 1.1
 * @date 2025-03-24
 *
 * @copyright Copyright (c) 2025
 *
 */
#pragma once

#include <array>
#include <tuple>

#include "coro/attribute.hpp"
#include "coro/comp/latch.hpp"
#include "coro/concepts/awaitable.hpp"

namespace coro
{
namespace detail
{

template<typename T>
class when_all_ready_awaitable;

template<>
class when_all_ready_awaitable<std::tuple<>>
{
public:
    constexpr when_all_ready_awaitable() noexcept {}
    explicit constexpr when_all_ready_awaitable(std::tuple<>) noexcept {}

    constexpr auto await_ready() const noexcept -> bool { return true; }
    constexpr auto await_suspend(std::coroutine_handle<>) noexcept -> bool { return false; }
    constexpr auto await_resume() const noexcept -> std::tuple<> { return {}; }
};

template<typename... task_types>
class when_all_ready_awaitable_base
{
public:
    explicit when_all_ready_awaitable_base(task_types&&... tasks) noexcept
        : m_latch(sizeof...(task_types)),
          m_tasks(std::move(tasks)...)
    {
    }

    explicit when_all_ready_awaitable_base(std::tuple<task_types...>&& tasks) noexcept
        : m_latch(sizeof...(task_types)),
          m_tasks(std::move(tasks))
    {
    }

    CORO_NO_COPY_MOVE(when_all_ready_awaitable_base);

protected:
    latch                     m_latch;
    std::tuple<task_types...> m_tasks;
};

template<typename... task_types>
    requires(concepts::all_void_type<typename task_types::rt...>)
class when_all_ready_awaitable<std::tuple<task_types...>> : public when_all_ready_awaitable_base<task_types...>
{
public:
    using when_all_ready_awaitable_base<task_types...>::when_all_ready_awaitable_base;

    auto operator co_await() noexcept
    {
        std::apply([this](auto&&... tasks) { ((tasks.start(this->m_latch)), ...); }, this->m_tasks);
        return this->m_latch.wait();
    }
};

// https://www.open-std.org/jtc1/sc22/wg21/docs/cwg_active.html#1430
template<typename task_type, typename... task_types>
    requires(
        concepts::all_noref_pod<typename task_type::rt, typename task_types::rt...> &&
        concepts::all_same_type<typename task_type::rt, typename task_types::rt...>)
class when_all_ready_awaitable<std::tuple<task_type, task_types...>>
    : public when_all_ready_awaitable_base<task_type, task_types...>
{
public:
    using storage_type = std::array<typename task_type::rt, 1 + sizeof...(task_types)>;

    using when_all_ready_awaitable_base<task_type, task_types...>::when_all_ready_awaitable_base;

    auto operator co_await() noexcept
    {
        struct awaiter
        {
            auto await_ready() noexcept -> bool { return m_awaiter.await_ready(); }

            auto await_suspend(std::coroutine_handle<> awaiting_coroutine) noexcept -> bool
            {
                return m_awaiter.await_suspend(awaiting_coroutine);
            }

            auto await_resume() noexcept -> decltype(auto)
            {
                m_awaiter.await_resume();
                return std::move(m_data);
            }

            latch::event_t::awaiter m_awaiter;
            storage_type&           m_data;
        };

        std::apply(
            [this](auto&&... tasks)
            {
                size_t p{0};
                ((tasks.start(this->m_latch, &(this->m_data[p++]))), ...);
            },
            this->m_tasks);

        return awaiter{this->m_latch.wait(), m_data};
    }

private:
    storage_type m_data;
};

template<typename T>
concept when_all_task_return_type = std::is_void_v<T> || concepts::conventional_type<T>;

template<when_all_task_return_type T>
class when_all_task_promise;

template<when_all_task_return_type T>
class when_all_task;

template<when_all_task_return_type return_type>
class when_all_task_promise_base
{
public:
    using coroutine_handle_type = std::coroutine_handle<when_all_task_promise<return_type>>;

    when_all_task_promise_base() noexcept {}

    auto initial_suspend() noexcept -> std::suspend_always { return {}; }

    auto final_suspend() noexcept
    {
        struct completion_notifier
        {
            auto await_ready() const noexcept -> bool { return false; }
            auto await_suspend(coroutine_handle_type coroutine) const noexcept -> void
            {
                coroutine.promise().m_latch->count_down();
            }
            auto await_resume() const noexcept {}
        };

        return completion_notifier{};
    }

    auto unhandled_exception() noexcept
    {
        // Keep simple, just ignore exception
    }

    auto start(latch& l) noexcept { m_latch = &l; }

protected:
    latch* m_latch{nullptr};
};

template<concepts::conventional_type return_type>
class when_all_task_promise<return_type> : public when_all_task_promise_base<return_type>
{
public:
    using storage_type = std::add_pointer_t<return_type>;

    auto get_return_object() noexcept -> decltype(auto);

    auto return_value(return_type value) noexcept -> void { *m_data = value; }

    auto set_pointer(storage_type data_ptr) noexcept -> void { m_data = data_ptr; }

private:
    storage_type m_data;
};

template<>
class when_all_task_promise<void> : public when_all_task_promise_base<void>
{
public:
    auto get_return_object() noexcept -> decltype(auto);

    constexpr auto return_void() noexcept -> void {}
};

template<when_all_task_return_type return_type>
class when_all_task
{
public:
    using promise_type          = when_all_task_promise<return_type>;
    using coroutine_handle_type = typename promise_type::coroutine_handle_type;
    using storage_type          = std::add_pointer_t<return_type>;
    using rt                    = return_type;

    explicit when_all_task(coroutine_handle_type handle) noexcept : m_handle(handle) {}
    when_all_task(const when_all_task&) = delete;
    when_all_task(when_all_task&& other) noexcept : m_handle(std::exchange(other.m_handle, nullptr)) {}

    when_all_task& operator=(const when_all_task&) = delete;
    when_all_task& operator=(when_all_task&&)      = delete;

    ~when_all_task() noexcept
    {
        if (m_handle != nullptr)
        {
            m_handle.destroy();
        }
    }

    void start(latch& l, storage_type p = nullptr) noexcept
    {
        if constexpr (!std::is_void_v<return_type>)
        {
            m_handle.promise().set_pointer(p);
        }
        m_handle.promise().start(l);
        submit_to_context(m_handle);
    }

private:
    coroutine_handle_type m_handle;
};

template<
    concepts::awaitable awaitable,
    typename return_type = typename concepts::awaitable_traits<awaitable&&>::awaiter_return_type>
[[CORO_AWAIT_HINT]] static auto make_when_all_task(awaitable&& a) -> when_all_task<return_type>
{
    if constexpr (std::is_void_v<return_type>)
    {
        co_await static_cast<awaitable&&>(a);
        co_return;
    }
    else
    {
        co_return co_await static_cast<awaitable&&>(a);
    }
}

}; // namespace detail

template<concepts::awaitable... awaitables_type>
[[CORO_TEST_USED(lab5a)]] [[CORO_AWAIT_HINT]] static auto when_all(awaitables_type... awaitables) noexcept
    -> decltype(auto)
{
    return detail::when_all_ready_awaitable<std::tuple<detail::when_all_task<
        std::remove_reference_t<typename concepts::awaitable_traits<awaitables_type>::awaiter_return_type>>...>>(
        std::make_tuple(detail::make_when_all_task(std::move(awaitables))...));
}

namespace detail
{
template<concepts::conventional_type return_type>
auto when_all_task_promise<return_type>::get_return_object() noexcept -> decltype(auto)
{
    return when_all_task<return_type>{
        when_all_task_promise_base<return_type>::coroutine_handle_type::from_promise(*this)};
}

auto when_all_task_promise<void>::get_return_object() noexcept -> decltype(auto)
{
    return when_all_task<void>{when_all_task_promise_base<void>::coroutine_handle_type::from_promise(*this)};
}

}; // namespace detail

}; // namespace coro
