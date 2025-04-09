#pragma once

#include <atomic>

#include "coro/attribute.hpp"

namespace coro
{
class context;
}; // namespace coro

namespace coro::detail
{
using config::ctx_id;
using std::atomic;
class engine;

/**
 * @brief store thread local variables
 *
 */
struct CORO_ALIGN local_info
{
    context* ctx{nullptr};
    engine*  egn{nullptr};
    // TODO: Add more local var
};

/**
 * @brief store thread shared variables
 *
 */
struct global_info
{
    atomic<ctx_id>   context_id{0};
    atomic<uint32_t> engine_id{0};
    // TODO: Add more global var
};

inline thread_local local_info linfo;
inline global_info             ginfo;

// init global info
inline auto init_meta_info() noexcept -> void
{
    ginfo.context_id = 0;
    ginfo.engine_id  = 0;
}
}; // namespace coro::detail