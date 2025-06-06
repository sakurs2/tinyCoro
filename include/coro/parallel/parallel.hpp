/**
 * @file parallel.hpp
 * @author Jiahui Wang
 * @brief Convenient and friendly parallel computing functions provided for users.
 * @version 1.2
 * @date 2025-06-03
 *
 * @copyright Copyright (c) 2025
 *
 */
#pragma once

#include <functional>

#include "coro/comp/when_all.hpp"
#include "coro/concepts/awaitable.hpp"
#include "coro/concepts/function_traits.hpp"

namespace coro::parallel
{
namespace detail
{
template<typename type>
concept parallel_tasks_void_type =
    std::ranges::range<type> && ::coro::concepts::awaitable_void<std::ranges::range_value_t<type>>;

template<typename type>
concept parallel_tasks_novoid_type =
    std::ranges::range<type> && ::coro::concepts::awaitable<std::ranges::range_value_t<type>>;

template<typename type>
concept reduce_operator_input = std::ranges::range<type>;
}; // namespace detail

// notes: reduce func is used to collect result, like mapreduce

/**
 * @brief Parallel calc but no reduce func
 *
 * @param tasks subtask collections
 * @return task<void>
 */
template<detail::parallel_tasks_void_type tasks_type>
auto parallel_func(tasks_type&& tasks) -> task<void>
{
    co_await when_all(std::forward<tasks_type>(tasks));
}

/**
 * @brief Convert any funciton object to std::function
 *
 * @tparam func_type
 */
template<typename func_type>
requires(coro::concepts::function_traits<func_type>::arity == 1) auto make_parallel_reduce_func(func_type func)
{
    using std_func_type = std::function<typename coro::concepts::function_traits<func_type>::return_type(
        typename coro::concepts::function_traits<func_type>::args<0>::type)>;
    return std_func_type(func);
}

/**
 * @brief Parallel calc with reduce func to collect result
 *
 * @tparam tasks_type
 * @tparam input_type: the input parameter of reduce func
 * @tparam return_type
 * @tparam ::coro::concepts::awaitable_traits<std::ranges::range_value_t<tasks_type>>::awaiter_return_type
 */
template<
    detail::parallel_tasks_novoid_type tasks_type,
    detail::reduce_operator_input      input_type,
    typename return_type,
    typename task_return_type =
        typename ::coro::concepts::awaitable_traits<std::ranges::range_value_t<tasks_type>>::awaiter_return_type>
requires(std::is_same_v<task_return_type, std::ranges::range_value_t<input_type>>) auto parallel_func(
    tasks_type&& tasks, std::function<return_type(input_type)> reduce) -> task<return_type>
{
    auto result = co_await when_all(std::forward<tasks_type>(tasks));
    co_return reduce(result);
}

}; // namespace coro::parallel
