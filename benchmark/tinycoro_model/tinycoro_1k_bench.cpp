#include "coro/coro.hpp"

using namespace coro;

#define BUFFLEN 1024 + 16

task<> session(int fd)
{
    char buf[BUFFLEN] = {0};
    auto conn         = io::net::tcp::tcp_connector(fd);
    int  ret          = 0;

    while ((ret = co_await conn.read(buf, BUFFLEN)) > 0)
    {
        ret = co_await conn.write(buf, ret);
        if (ret <= 0)
        {
            break;
        }
    }

    ret = co_await conn.close();
    assert(ret == 0);
}

task<> server(int port)
{
    auto server = io::net::tcp::tcp_server(port);
    log::info("server start in {}", port);
    int client_fd;
    while ((client_fd = co_await server.accept()) > 0)
    {
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
