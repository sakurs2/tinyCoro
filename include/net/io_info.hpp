#pragma once

#include <coroutine>
#include <cstdint>

namespace coro
{
#define PTRCAST(data) reinterpret_cast<uintptr_t>(data)
#define DATACAST(data) static_cast<uintptr_t>(data)

  using std::coroutine_handle;
  enum Tasktype
  {
    TcpAccept,
    TcpConnect,
    TcpRead,
    TcpWrite,
    TcpClose,
    Stdin,
    None
  };

  struct IoInfo
  {
    coroutine_handle<> handle;
    int32_t result;
    Tasktype type;
    uintptr_t data;
  };

  inline uintptr_t ioinfo_to_ptr(IoInfo *info) noexcept
  {
    return reinterpret_cast<uintptr_t>(info);
  }

  inline IoInfo *ptr_to_ioinfo(uintptr_t ptr) noexcept
  {
    return reinterpret_cast<IoInfo *>(ptr);
  }
};