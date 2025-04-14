#include "coro/comp/condition_variable.hpp"
#include "coro/scheduler.hpp"

namespace coro
{
auto condition_variable::cv_awaiter::await_suspend(std::coroutine_handle<> handle) noexcept -> bool
{
    m_await_coro = handle;
    return register_lock();
}

auto condition_variable::cv_awaiter::await_resume() noexcept -> void
{
    m_ctx.unregister_wait(m_suspend_state);
}

auto condition_variable::cv_awaiter::register_lock() noexcept -> bool
{
    if (m_cond && m_cond())
    {
        return false;
    }

    m_ctx.register_wait(!m_suspend_state);
    m_suspend_state = true;

    register_cv();
    m_mtx.unlock();
    return true;
}

auto condition_variable::cv_awaiter::register_cv() noexcept -> void
{
    m_next = nullptr;

    m_cv.m_lock.lock();
    if (m_cv.m_tail == nullptr)
    {
        m_cv.m_head = m_cv.m_tail = this;
    }
    else
    {
        m_cv.m_tail->m_next = this;
        m_cv.m_tail         = this;
    }
    m_cv.m_lock.unlock();
}

auto condition_variable::cv_awaiter::wake_up() noexcept -> void
{
    if (!mutex_awaiter::register_lock())
    {
        resume();
    }
}

auto condition_variable::cv_awaiter::resume() noexcept -> void
{
    if (m_cond && !m_cond())
    {
        m_ctx.register_wait(!m_suspend_state);
        m_suspend_state = true;

        register_cv();
        m_mtx.unlock();
        return;
    }
    mutex_awaiter::resume();
}

condition_variable::~condition_variable() noexcept
{
    assert(m_head == nullptr && m_tail == nullptr && "exist sleep awaiter when cv destruct");
}

auto condition_variable::wait(mutex& mtx) noexcept -> cv_awaiter
{
    return cv_awaiter(local_context(), mtx, *this);
}

auto condition_variable::wait(mutex& mtx, cond_type&& cond) noexcept -> cv_awaiter
{
    return cv_awaiter(local_context(), mtx, *this, cond);
}

auto condition_variable::wait(mutex& mtx, cond_type& cond) noexcept -> cv_awaiter
{
    return cv_awaiter(local_context(), mtx, *this, cond);
}

// https://stackoverflow.com/questions/17101922/do-i-have-to-acquire-lock-before-calling-condition-variable-notify-one
auto condition_variable::notify_one() noexcept -> void
{
    m_lock.lock();
    auto cur = m_head;
    if (cur != nullptr)
    {
        m_head = reinterpret_cast<cv_awaiter*>(m_head->m_next);
        if (m_head == nullptr)
        {
            m_tail = nullptr;
        }
        m_lock.unlock();
        cur->wake_up();
    }
    else
    {
        m_lock.unlock();
    }
}

auto condition_variable::notify_all() noexcept -> void
{
    cv_awaiter* nxt{nullptr};

    m_lock.lock();
    auto cur_head = m_head;
    m_head = m_tail = nullptr;
    m_lock.unlock();

    while (cur_head != nullptr)
    {
        nxt = reinterpret_cast<cv_awaiter*>(cur_head->m_next);
        cur_head->wake_up();
        cur_head = nxt;
    }
}
} // namespace coro
