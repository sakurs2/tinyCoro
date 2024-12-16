#include <cassert>

#include "lock/mutex.hpp"
#include "coro/context.hpp"

// TODO: use memory order
namespace coro
{
  bool Mutex::LockAwaiter::register_lock()
  {
    while (true)
    {
      auto state = mtx_.state_.load();
      next_ = nullptr;
      if (state == Mutex::nolocked)
      {
        if (mtx_.state_.compare_exchange_strong(
                state, Mutex::locked_no_waiting))
        {
          return false;
        }
      }
      else
      {
        next_ = reinterpret_cast<LockAwaiter *>(state);
        if (mtx_.state_.compare_exchange_strong(
                state, reinterpret_cast<uintptr_t>(this)))
        {
          return true;
        }
      }
    }
  }

  void Mutex::LockAwaiter::submit_task()
  {
    local_thread_context()->submit_task(wait_handle_);
  }

  Mutex::~Mutex()
  {
    [[__attribute_maybe_unused__]] auto state = state_.load();
    assert(state == nolocked);
  }

  bool Mutex::try_lock()
  {
    auto target = nolocked;
    return state_.compare_exchange_strong(target, locked_no_waiting);
  }

  void Mutex::unlock()
  {
    assert(state_.load() != nolocked);
    LockAwaiter *awaiter{nullptr};
    while (true)
    {
      auto target = locked_no_waiting;
      if (state_.compare_exchange_strong(target, nolocked))
      {
        return;
      }
      auto state = state_.load();
      awaiter = reinterpret_cast<LockAwaiter *>(state);
      auto nxt = reinterpret_cast<uintptr_t>(awaiter->next_);
      if (state_.compare_exchange_strong(state, nxt))
      {
        break;
      }
    }
    if (awaiter != nullptr)
    {
      awaiter->submit_task();
    }
  }

};