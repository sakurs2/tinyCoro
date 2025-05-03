#include <optional>
#include <sys/socket.h>
#include <unistd.h>

#include "coro/net/io_awaiter.hpp"
#include "coro/scheduler.hpp"

namespace coro::net
{
using ::coro::detail::local_engine;
using detail::io_type;

noop_awaiter::noop_awaiter() noexcept
{
    m_info.type = io_type::nop;
    m_info.cb   = &noop_awaiter::callback;

    io_uring_prep_nop(m_urs);
    io_uring_sqe_set_data(m_urs, &m_info);
    local_engine().add_io_submit();
}

auto noop_awaiter::callback(io_info* data, int res) noexcept -> void
{
    data->result = res;
    submit_to_context(data->handle);
}

tcp_accept_awaiter::tcp_accept_awaiter(int listenfd, int io_flag, int sqe_flag) noexcept
{
    m_info.type = io_type::tcp_accept;
    m_info.cb   = &tcp_accept_awaiter::callback;

    // FIXME: this isn't atomic, maybe cause bug?
    io_uring_sqe_set_flags(m_urs, sqe_flag);
    io_uring_prep_accept(m_urs, listenfd, nullptr, &len, io_flag);
    io_uring_sqe_set_data(m_urs, &m_info); // old uring version need set data after prep
    local_engine().add_io_submit();
}

auto tcp_accept_awaiter::callback(io_info* data, int res) noexcept -> void
{
    data->result = res;
    submit_to_context(data->handle);
}

tcp_read_awaiter::tcp_read_awaiter(int sockfd, char* buf, size_t len, int io_flag, int sqe_flag) noexcept
{
    m_info.type = io_type::tcp_read;
    m_info.cb   = &tcp_read_awaiter::callback;

    io_uring_sqe_set_flags(m_urs, sqe_flag);
    io_uring_prep_recv(m_urs, sockfd, buf, len, io_flag);
    io_uring_sqe_set_data(m_urs, &m_info);
    local_engine().add_io_submit();
}

auto tcp_read_awaiter::callback(io_info* data, int res) noexcept -> void
{
    data->result = res;
    submit_to_context(data->handle);
}

tcp_write_awaiter::tcp_write_awaiter(int sockfd, char* buf, size_t len, int io_flag, int sqe_flag) noexcept
{
    m_info.type = io_type::tcp_write;
    m_info.cb   = &tcp_write_awaiter::callback;

    io_uring_sqe_set_flags(m_urs, sqe_flag);
    io_uring_prep_send(m_urs, sockfd, buf, len, io_flag);
    io_uring_sqe_set_data(m_urs, &m_info);
    local_engine().add_io_submit();
}

auto tcp_write_awaiter::callback(io_info* data, int res) noexcept -> void
{
    data->result = res;
    submit_to_context(data->handle);
}

tcp_close_awaiter::tcp_close_awaiter(int sockfd) noexcept
{
    m_info.type = io_type::tcp_close;
    m_info.cb   = &tcp_close_awaiter::callback;

    io_uring_prep_close(m_urs, sockfd);
    io_uring_sqe_set_data(m_urs, &m_info);
    local_engine().add_io_submit();
}

auto tcp_close_awaiter::callback(io_info* data, int res) noexcept -> void
{
    data->result = res;
    submit_to_context(data->handle);
}

tcp_connect_awaiter::tcp_connect_awaiter(int sockfd, const sockaddr* addr, socklen_t addrlen) noexcept
{
    m_info.type = io_type::tcp_connect;
    m_info.cb   = &tcp_connect_awaiter::callback;
    m_info.data = CASTDATA(sockfd);

    io_uring_prep_connect(m_urs, sockfd, addr, addrlen);
    io_uring_sqe_set_data(m_urs, &m_info);
    local_engine().add_io_submit();
}

auto tcp_connect_awaiter::callback(io_info* data, int res) noexcept -> void
{
    if (res != 0)
    {
        data->result = res;
    }
    else
    {
        data->result = static_cast<int>(data->data);
    }
    submit_to_context(data->handle);
}

stdin_awaiter::stdin_awaiter(char* buf, size_t len, int io_flag, int sqe_flag) noexcept
{
    m_info.type = io_type::stdin;
    m_info.cb   = &stdin_awaiter::callback;

    io_uring_sqe_set_flags(m_urs, sqe_flag);
    io_uring_prep_read(m_urs, STDIN_FILENO, buf, len, io_flag);
    io_uring_sqe_set_data(m_urs, &m_info);
    local_engine().add_io_submit();
}

auto stdin_awaiter::callback(io_info* data, int res) noexcept -> void
{
    data->result = res;
    submit_to_context(data->handle);
}

}; // namespace coro::net