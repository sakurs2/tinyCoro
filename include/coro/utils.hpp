#pragma once

#include <chrono>
#include <cstdint>
#include <string>
#include <thread>

namespace coro::utils
{
/**
 * @brief set the fd noblock
 *
 * @param fd
 */
auto set_fd_noblock(int fd) noexcept -> void;

auto get_null_fd() noexcept -> int;

inline auto sleep(int64_t t) noexcept -> void
{
    std::this_thread::sleep_for(std::chrono::seconds(t));
}

inline auto msleep(int64_t t) noexcept -> void
{
    std::this_thread::sleep_for(std::chrono::milliseconds(t));
}

inline auto usleep(int64_t t) noexcept -> void
{
    std::this_thread::sleep_for(std::chrono::microseconds(t));
}

/**
 * @brief remove to_trim from s
 *
 * @param s
 * @param to_trim
 * @return std::string&
 */
auto trim(std::string& s, const char* to_trim) noexcept -> std::string&;
}; // namespace coro::utils