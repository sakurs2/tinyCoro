#pragma once

#include <netdb.h>

#include "coro/net/base_awaiter.hpp"

namespace coro::net
{
using ::coro::net::detail::io_info;

class noop_awaiter : public detail::base_io_awaiter
{
public:
    noop_awaiter() noexcept;
    static auto callback(io_info* data, int res) noexcept -> void;
};

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

class stdin_awaiter : public detail::base_io_awaiter
{
public:
    stdin_awaiter(char* buf, size_t len, int io_flag = 0, int sqe_flag = 0) noexcept;

    static auto callback(io_info* data, int res) noexcept -> void;
};

}; // namespace coro::net
