#include <atomic>

#include "config.h"

namespace coro::detail
{
/**
 * @brief std::vector<std::atomic<T>> will cause compile error, so use atomic_ref_wrapper
 *
 * @tparam T
 */
template<typename T>
struct alignas(config::kCacheLineSize) atomic_ref_wrapper
{
    alignas(std::atomic_ref<T>::required_alignment) T val;
};

}; // namespace coro::detail