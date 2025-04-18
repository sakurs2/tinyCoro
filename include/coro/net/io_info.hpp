#pragma once

#include <coroutine>
#include <cstdint>
#include <functional>

namespace coro::net::detail
{
#define CASTPTR(data)  reinterpret_cast<uintptr_t>(data)
#define CASTDATA(data) static_cast<uintptr_t>(data)

struct io_info;

using std::coroutine_handle;
using cb_type = std::function<void(io_info*, int)>;

enum io_type
{
    nop,
    tcp_accept,
    tcp_connect,
    tcp_read,
    tcp_write,
    tcp_close,
    stdin,
    timer,
    none
};

struct io_info
{
    coroutine_handle<> handle;
    int32_t            result;
    io_type            type;
    uintptr_t          data;
    cb_type            cb;
};

inline uintptr_t ioinfo_to_ptr(io_info* info) noexcept
{
    return reinterpret_cast<uintptr_t>(info);
}

inline io_info* ptr_to_ioinfo(uintptr_t ptr) noexcept
{
    return reinterpret_cast<io_info*>(ptr);
}

}; // namespace coro::net::detail