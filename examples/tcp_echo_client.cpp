#include "coro/coro.hpp"

using namespace coro;

#define BUFFLEN 10240

task<> client(const char* addr, int port)
{
    log::info("client ready to start");
    auto client = io::net::tcp::tcp_client(addr, port);
    int  ret    = 0;
    int  sockfd = 0;
    sockfd      = co_await client.connect();
    assert(sockfd > 0 && "connect error");
    log::info("connect success");

    char buf[BUFFLEN] = {0};
    auto conn         = io::net::tcp::tcp_connector(sockfd);
    while ((ret = co_await conn.read(buf, BUFFLEN)) > 0)
    {
        log::info("client {} receive data: {}", sockfd, buf);
        ret = co_await conn.write(buf, ret);
    }

    ret = co_await conn.close();
    log::info("client close");
    assert(ret == 0);
}

int main(int argc, char const* argv[])
{
    /* code */
    scheduler::init();
    scheduler::submit(client("localhost", 8000));
    scheduler::loop();
    return 0;
}
