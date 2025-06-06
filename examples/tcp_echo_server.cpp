#include "coro/coro.hpp"

using namespace coro;

#define BUFFLEN 10240

task<> session(int fd)
{
    char buf[BUFFLEN] = {0};
    auto conn         = io::net::tcp::tcp_connector(fd);
    int  ret          = 0;
    while ((ret = co_await conn.read(buf, BUFFLEN)) > 0)
    {
        log::info("client {} receive data: {}", fd, buf);
        ret = co_await conn.write(buf, ret);
    }

    ret = co_await conn.close();
    log::info("client {} close connect", fd);
    assert(ret == 0);
}

task<> server(int port)
{
    log::info("server start in {}", port);
    auto server = io::net::tcp::tcp_server(port);
    int  client_fd;
    while ((client_fd = co_await server.accept()) > 0)
    {
        log::info("server receive new connect");
        submit_to_scheduler(session(client_fd));
    }
}

int main(int argc, char const* argv[])
{
    /* code */
    scheduler::init();

    submit_to_scheduler(server(8000));
    scheduler::loop();
    return 0;
}
