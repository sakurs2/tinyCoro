/**
 * @file io_awaiter.hpp
 * @author Jiahui Wang
 * @brief This file includes all awaiter of IO operations
 * @version 1.2
 * @date 2025-05-27
 *
 * @copyright Copyright (c) 2025
 *
 */
#pragma once

#include <netdb.h>

#include "coro/io/base_awaiter.hpp"

namespace coro::io
{
using ::coro::io::detail::io_info;

class noop_awaiter : public detail::base_io_awaiter
{
public:
    noop_awaiter() noexcept;
    static auto callback(io_info* data, int res) noexcept -> void;
};

class stdin_awaiter : public detail::base_io_awaiter
{
public:
    stdin_awaiter(char* buf, size_t len, int io_flag = 0, int sqe_flag = 0) noexcept;

    static auto callback(io_info* data, int res) noexcept -> void;
};

namespace net
{
/**
 * @brief tcp awaiter
 *
 */
namespace tcp
{
class tcp_accept_awaiter : public detail::base_io_awaiter
{
public:
    tcp_accept_awaiter(int listenfd, int io_flag = 0, int sqe_flag = 0) noexcept;

    static auto callback(io_info* data, int res) noexcept -> void;

private:
    inline static socklen_t len = sizeof(sockaddr_in);
};

class tcp_read_awaiter : public detail::base_io_awaiter
{
public:
    tcp_read_awaiter(int sockfd, char* buf, size_t len, int io_flag = 0, int sqe_flag = 0) noexcept;

    static auto callback(io_info* data, int res) noexcept -> void;
};

class tcp_write_awaiter : public detail::base_io_awaiter
{
public:
    tcp_write_awaiter(int sockfd, char* buf, size_t len, int io_flag = 0, int sqe_flag = 0) noexcept;

    static auto callback(io_info* data, int res) noexcept -> void;
};

class tcp_close_awaiter : public detail::base_io_awaiter
{
public:
    tcp_close_awaiter(int sockfd) noexcept;

    static auto callback(io_info* data, int res) noexcept -> void;
};

class tcp_connect_awaiter : public detail::base_io_awaiter
{
public:
    tcp_connect_awaiter(int sockfd, const sockaddr* addr, socklen_t addrlen) noexcept;

    static auto callback(io_info* data, int res) noexcept -> void;
};
}; // namespace tcp
}; // namespace net

}; // namespace coro::io
