#pragma once

#include <arpa/inet.h>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "config.h"
#include "coro/net/base_io_type.hpp"
#include "coro/net/io_awaiter.hpp"

namespace coro::net
{

class tcp_connector
{
public:
    explicit tcp_connector(int sockfd) noexcept : m_sockfd(sockfd), m_original_fd(sockfd), m_sqe_flag(0)
    {
        m_fixed_fd.assign(m_sockfd, m_sqe_flag);
    }

    tcp_read_awaiter read(char* buf, size_t len, int io_flags = 0) noexcept
    {
        return tcp_read_awaiter(m_sockfd, buf, len, io_flags, m_sqe_flag);
    }

    tcp_write_awaiter write(char* buf, size_t len, int io_flags = 0) noexcept
    {
        return tcp_write_awaiter(m_sockfd, buf, len, io_flags, m_sqe_flag);
    }

    // close() must use original sock fd
    tcp_close_awaiter close() noexcept
    {
        m_fixed_fd.return_back();
        return tcp_close_awaiter(m_original_fd);
    }

private:
    int       m_sockfd;      // Can be converted to fix fd
    const int m_original_fd; // original sock fd

    detail::fixed_fds m_fixed_fd;
    int               m_sqe_flag;
};

class tcp_server
{
public:
    explicit tcp_server(int port = ::coro::config::kDefaultPort) noexcept : tcp_server(nullptr, port) {}

    tcp_server(const char* addr, int port) noexcept;

    tcp_accept_awaiter accept(int io_flags = 0) noexcept;

private:
    int         m_listenfd;
    int         m_port;
    sockaddr_in m_servaddr;

    detail::fixed_fds m_fixed_fd;
    int               m_sqe_flag{0};
};

class tcp_client
{
public:
    tcp_client(const char* addr, int port) noexcept;

    tcp_connect_awaiter connect() noexcept;

private:
    int         m_clientfd;
    int         m_port;
    sockaddr_in m_servaddr;
};

}; // namespace coro::net
