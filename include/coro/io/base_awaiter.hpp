#pragma once

#include <coroutine>

#include "coro/context.hpp"
#include "coro/io/io_info.hpp"
#include "coro/uring_proxy.hpp"

namespace coro::io::detail
{

class base_io_awaiter
{
public:
    base_io_awaiter() noexcept : m_urs(coro::detail::local_engine().get_free_urs())
    {
        // TODO: you should no-block wait when get_free_urs return nullptr,
        // this means io submit rate is too high.
        assert(m_urs != nullptr && "io submit rate is too high");
    }

    constexpr auto await_ready() noexcept -> bool { return false; }

    auto await_suspend(std::coroutine_handle<> handle) noexcept -> void { m_info.handle = handle; }

    auto await_resume() noexcept -> int32_t { return m_info.result; }

protected:
    io_info             m_info;
    coro::uring::ursptr m_urs;
};

}; // namespace coro::io::detail