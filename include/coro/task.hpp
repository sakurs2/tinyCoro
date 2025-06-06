#pragma once

#include <cassert>
#include <coroutine>
#include <stdexcept>
#include <utility>

#include "coro/attribute.hpp"
#include "coro/detail/container.hpp"

#ifdef ENABLE_MEMORY_ALLOC
    #include "coro/meta_info.hpp"
#endif

namespace coro
{
template<typename return_type = void>
class task;

namespace detail
{
enum class coro_state : uint8_t
{
    normal,
    detach,
    cancel, // TODO: Impl cancel
    none
};
// TODO: Add carefully control for lvalue call and rvalue call
// TODO: Add yield value support
struct promise_base
{
    friend struct final_awaitable;
    struct final_awaitable
    {
        constexpr auto await_ready() const noexcept -> bool { return false; }

        template<typename promise_type>
        auto await_suspend(std::coroutine_handle<promise_type> coroutine) noexcept -> std::coroutine_handle<>
        {
            // If there is a continuation call it, otherwise this is the end of the line.
            auto& promise = coroutine.promise();
            return promise.m_continuation != nullptr ? promise.m_continuation : std::noop_coroutine();
        }

        constexpr auto await_resume() noexcept -> void {}
    };

    promise_base() noexcept = default;
    ~promise_base()         = default;

    constexpr auto initial_suspend() noexcept { return std::suspend_always{}; }

    [[CORO_TEST_USED(lab1)]] constexpr auto final_suspend() noexcept { return final_awaitable{}; }

    auto continuation(std::coroutine_handle<> continuation) noexcept -> void { m_continuation = continuation; }

    inline auto set_state(coro_state state) -> void { m_state = state; }

    inline auto get_state() -> coro_state { return m_state; }

    inline auto is_detach() -> bool { return m_state == coro_state::detach; }

#ifdef ENABLE_MEMORY_ALLOC
    void* operator new(std::size_t size)
    {
        return ::coro::detail::ginfo.mem_alloc->allocate(size);
    }

    void operator delete(void* ptr, [[CORO_MAYBE_UNUSED]] std::size_t size)
    {
        ::coro::detail::ginfo.mem_alloc->release(ptr);
    }
#endif

protected:
    std::coroutine_handle<> m_continuation{nullptr};
    coro_state              m_state{coro_state::normal};

#ifdef DEBUG
public:
    int promise_id{0};
#endif // DEBUG
};

template<typename return_type>
struct promise final : public promise_base, public container<return_type>
{
public:
    using task_type        = task<return_type>;
    using coroutine_handle = std::coroutine_handle<promise<return_type>>;

#ifdef DEBUG
    template<typename... Args>
    promise(int id, Args&&... args) noexcept
    {
        promise_id = id;
    }
#endif // DEBUG
    promise() noexcept
    {
    }
    promise(const promise&)             = delete;
    promise(promise&& other)            = delete;
    promise& operator=(const promise&)  = delete;
    promise& operator=(promise&& other) = delete;
    ~promise()                          = default;

    auto get_return_object() noexcept -> task_type;

    auto unhandled_exception() noexcept -> void
    {
        this->set_exception();
    }
};

template<>
struct promise<void> : public promise_base
{
    using task_type        = task<void>;
    using coroutine_handle = std::coroutine_handle<promise<void>>;

#ifdef DEBUG
    template<typename... Args>
    promise(int id, Args&&... args) noexcept
    {
        promise_id = id;
    }
#endif // DEBUG
    promise() noexcept                  = default;
    promise(const promise&)             = delete;
    promise(promise&& other)            = delete;
    promise& operator=(const promise&)  = delete;
    promise& operator=(promise&& other) = delete;
    ~promise()                          = default;

    auto get_return_object() noexcept -> task_type;

    constexpr auto return_void() noexcept -> void
    {
    }

    auto unhandled_exception() noexcept -> void
    {
        m_exception_ptr = std::current_exception();
    }

    auto result() -> void
    {
        if (m_exception_ptr)
        {
            std::rethrow_exception(m_exception_ptr);
        }
    }

private:
    std::exception_ptr m_exception_ptr{nullptr};
};

} // namespace detail

template<typename return_type>
class [[CORO_AWAIT_HINT]] task
{
public:
    using task_type        = task<return_type>;
    using promise_type     = detail::promise<return_type>;
    using coroutine_handle = std::coroutine_handle<promise_type>;

    struct awaitable_base
    {
        awaitable_base(coroutine_handle coroutine) noexcept : m_coroutine(coroutine) {}

        auto await_ready() const noexcept -> bool { return !m_coroutine || m_coroutine.done(); }

        auto await_suspend(std::coroutine_handle<> awaiting_coroutine) noexcept -> std::coroutine_handle<>
        {
            m_coroutine.promise().continuation(awaiting_coroutine);
            return m_coroutine;
        }

        std::coroutine_handle<promise_type> m_coroutine{nullptr};
    };

    task() noexcept : m_coroutine(nullptr) {}

    explicit task(coroutine_handle handle) : m_coroutine(handle) {}
    task(const task&) = delete;
    task(task&& other) noexcept : m_coroutine(std::exchange(other.m_coroutine, nullptr)) {}

    ~task()
    {
        if (m_coroutine != nullptr)
        {
            m_coroutine.destroy();
        }
    }

    auto operator=(const task&) -> task& = delete;

    auto operator=(task&& other) noexcept -> task&
    {
        if (std::addressof(other) != this)
        {
            if (m_coroutine != nullptr)
            {
                m_coroutine.destroy();
            }

            m_coroutine = std::exchange(other.m_coroutine, nullptr);
        }

        return *this;
    }

    /**
     * @return True if the task is in its final suspend or if the task has been destroyed.
     */
    auto is_ready() const noexcept -> bool { return m_coroutine == nullptr || m_coroutine.done(); }

    auto resume() -> bool
    {
        if (!m_coroutine.done())
        {
            m_coroutine.resume();
        }
        return !m_coroutine.done();
    }

    auto destroy() -> bool
    {
        if (m_coroutine != nullptr)
        {
            m_coroutine.destroy();
            m_coroutine = nullptr;
            return true;
        }

        return false;
    }

    [[CORO_TEST_USED(lab1)]] auto detach() -> void
    {
        assert(m_coroutine != nullptr && "detach func expected no-nullptr coroutine_handler");
        auto& promise = m_coroutine.promise();
        promise.set_state(detail::coro_state::detach);
        m_coroutine = nullptr;
    }

    auto operator co_await() const& noexcept
    {
        struct awaitable : public awaitable_base
        {
            auto await_resume() -> decltype(auto) { return this->m_coroutine.promise().result(); }
        };

        return awaitable{m_coroutine};
    }

    auto operator co_await() const&& noexcept
    {
        struct awaitable : public awaitable_base
        {
            auto await_resume() -> decltype(auto) { return std::move(this->m_coroutine.promise()).result(); }
        };

        return awaitable{m_coroutine};
    }

    auto promise() & -> promise_type& { return m_coroutine.promise(); }
    auto promise() const& -> const promise_type& { return m_coroutine.promise(); }
    auto promise() && -> promise_type&& { return std::move(m_coroutine.promise()); }

    auto handle() & -> coroutine_handle { return m_coroutine; }
    auto handle() && -> coroutine_handle { return std::exchange(m_coroutine, nullptr); }

private:
    coroutine_handle m_coroutine{nullptr};
};

using coroutine_handle = std::coroutine_handle<detail::promise_base>;

/**
 * @brief do clean work when handle is done
 *
 * @param handle
 */
[[CORO_TEST_USED(lab1)]] inline auto clean(std::coroutine_handle<> handle) noexcept -> void
{
    auto  specific_handle = coroutine_handle::from_address(handle.address());
    auto& promise         = specific_handle.promise();
    switch (promise.get_state())
    {
        case detail::coro_state::detach:
            handle.destroy();
            break;
        default:
            break;
    }
}

namespace detail
{
template<typename return_type>
inline auto promise<return_type>::get_return_object() noexcept -> task<return_type>
{
    return task<return_type>{coroutine_handle::from_promise(*this)};
}

inline auto promise<void>::get_return_object() noexcept -> task<>
{
    return task<>{coroutine_handle::from_promise(*this)};
}

#ifdef DEBUG
template<typename T = void>
inline auto get_promise(std::coroutine_handle<> handle) -> promise<T>&
{
    auto  specific_handle = std::coroutine_handle<detail::promise<T>>::from_address(handle.address());
    auto& promise         = specific_handle.promise();
    return promise;
}
#endif // DEBUG

} // namespace detail

} // namespace coro
