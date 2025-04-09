/**
 * @file latch.hpp
 * @author JiahuiWang
 * @brief lab4b
 * @version 1.1
 * @date 2025-03-24
 *
 * @copyright Copyright (c) 2025
 *
 */
#pragma once

#include <atomic>

#include "coro/comp/event.hpp"

namespace coro
{

class latch
{
public:
    using event_t = event<>;
    latch(std::uint64_t count) noexcept : m_count(count), m_ev(count <= 0) {}
    latch(const latch&)                    = delete;
    latch(latch&&)                         = delete;
    auto operator=(const latch&) -> latch& = delete;
    auto operator=(latch&&) -> latch&      = delete;

    auto count_down() noexcept -> void
    {
        if (m_count.fetch_sub(1, std::memory_order::acq_rel) <= 1)
        {
            m_ev.set();
        }
    }

    auto wait() noexcept -> event_t::awaiter { return m_ev.wait(); }

private:
    std::atomic<std::int64_t> m_count;
    event_t                   m_ev;
};

class latch_guard
{
public:
    latch_guard(latch& l) noexcept : m_l(l) {}
    ~latch_guard() noexcept { m_l.count_down(); }

private:
    latch& m_l;
};

}; // namespace coro
