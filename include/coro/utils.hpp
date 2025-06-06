#pragma once

#include <chrono>
#include <cstdint>
#include <regex>
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

/**
 * @brief Get the nonsense fd, don't forget to close
 *
 * @return int
 */
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

/**
 * @brief Convert the letters to lowercase
 *
 * @param c
 * @return unsigned char
 */
inline auto to_lower(int c) -> unsigned char
{
    const static unsigned char table[256] = {
        0,   1,   2,   3,   4,   5,   6,   7,   8,   9,   10,  11,  12,  13,  14,  15,  16,  17,  18,  19,  20,  21,
        22,  23,  24,  25,  26,  27,  28,  29,  30,  31,  32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,
        44,  45,  46,  47,  48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  62,  63,  64,  97,
        98,  99,  100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119,
        120, 121, 122, 91,  92,  93,  94,  95,  96,  97,  98,  99,  100, 101, 102, 103, 104, 105, 106, 107, 108, 109,
        110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131,
        132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149, 150, 151, 152, 153,
        154, 155, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175,
        176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 224, 225, 226, 227, 228, 229,
        230, 231, 232, 233, 234, 235, 236, 237, 238, 239, 240, 241, 242, 243, 244, 245, 246, 215, 248, 249, 250, 251,
        252, 253, 254, 223, 224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239, 240, 241,
        242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254, 255,
    };
    return table[(unsigned char)(char)c];
}

/**
 * @brief Calculate the hash value of string
 *
 * @warning case insensitive
 */
struct hash
{
    auto operator()(const std::string& key) const -> size_t { return hash_core(key.data(), key.size(), 0); }

    auto hash_core(const char* s, size_t l, size_t h) const -> size_t
    {
        return (l == 0) ? h
                        : hash_core(
                              s + 1,
                              l - 1,
                              // Unsets the 6 high bits of h, therefore no
                              // overflow happens
                              (((std::numeric_limits<size_t>::max)() >> 6) & h * 33) ^
                                  static_cast<unsigned char>(to_lower(*s)));
    }
};

/**
 * @brief return if a == b
 *
 * @warning case insensitive
 */
inline auto equal(const std::string& a, const std::string& b) -> bool
{
    return a.size() == b.size() &&
           std::equal(a.begin(), a.end(), b.begin(), [](char ca, char cb) { return to_lower(ca) == to_lower(cb); });
}

struct equal_to
{
    auto operator()(const std::string& a, const std::string& b) const -> bool { return equal(a, b); }
};

/**
 * @brief Determine whether a string is a number
 *
 * @param str
 * @return true
 * @return false
 */
inline auto is_numeric(const std::string& str) -> bool
{
    return !str.empty() && std::all_of(str.cbegin(), str.cend(), [](unsigned char c) { return std::isdigit(c); });
}

/**
 * @brief Find file extension
 *
 * @param path
 * @return std::string
 */
inline auto file_extension(const std::string& path) -> std::string
{
    std::smatch       m;
    thread_local auto re = std::regex("\\.([a-zA-Z0-9]+)$");
    if (std::regex_search(path, m, re))
    {
        return m[1].str();
    }
    return std::string();
}
}; // namespace coro::utils