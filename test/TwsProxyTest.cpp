#ifndef _WIN32
#include "TwsProxy.h"
#include <gtest/gtest.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <atomic>
#include <cerrno>
#include <chrono>
#include <string>
#include <thread>
#include <vector>

/***
 * Tests for the TwsProxy test helper, with a small fake server in place of TWS.
 * None of these need a running TWS.
 */

namespace {

/*** stands in for TWS: echoes what it receives, or sends a fixed reply on connect */
class FakeServer
{
    public:
    explicit FakeServer(std::string greeting = "") : greeting(std::move(greeting))
    {
        fd = socket(AF_INET, SOCK_STREAM, 0);
        int one = 1;
        setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        bind(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
        listen(fd, 8);
        socklen_t len = sizeof(addr);
        getsockname(fd, reinterpret_cast<sockaddr*>(&addr), &len);
        port = ntohs(addr.sin_port);
        thread = std::thread([this]() { Run(); });
    }
    ~FakeServer()
    {
        stop = true;
        thread.join();
        close(fd);
    }
    int port = 0;

    private:
    void Run()
    {
        std::vector<std::thread> clients;
        while (!stop)
        {
            pollfd p{fd, POLLIN, 0};
            if (poll(&p, 1, 50) <= 0)
                continue;
            int c = accept(fd, nullptr, nullptr);
            if (c < 0)
                continue;
            clients.emplace_back([this, c]()
            {
                if (!greeting.empty())
                    send(c, greeting.data(), greeting.size(), 0);
                char buf[1024];
                while (!stop)
                {
                    pollfd p{c, POLLIN, 0};
                    if (poll(&p, 1, 50) <= 0)
                        continue;
                    ssize_t n = recv(c, buf, sizeof(buf), 0);
                    if (n <= 0)
                        break;
                    send(c, buf, n, 0);
                }
                close(c);
            });
        }
        for(auto& t : clients)
            t.join();
    }
    std::string greeting;
    int fd = -1;
    std::atomic<bool> stop = false;
    std::thread thread;
};

/*** @returns the socket, or -1 with errno set */
int ConnectLocal(int port)
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (::connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0)
    {
        int e = errno;
        close(fd);
        errno = e;
        return -1;
    }
    return fd;
}

/*** read until the peer closes, an error, or the timeout. @returns bytes read; error in err (0 = clean EOF) */
std::string ReadToEnd(int fd, int& err, int timeoutMs = 2000)
{
    std::string out;
    err = ETIMEDOUT;
    auto end = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
    char buf[1024];
    while (std::chrono::steady_clock::now() < end)
    {
        pollfd p{fd, POLLIN, 0};
        if (poll(&p, 1, 20) <= 0)
            continue;
        ssize_t n = recv(fd, buf, sizeof(buf), 0);
        if (n == 0)
        {
            err = 0;
            return out;
        }
        if (n < 0)
        {
            err = errno;
            return out;
        }
        out.append(buf, n);
    }
    return out;
}

std::string Echo(int fd, const std::string& msg)
{
    send(fd, msg.data(), msg.size(), 0);
    std::string out;
    char buf[1024];
    auto end = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    while (out.size() < msg.size() && std::chrono::steady_clock::now() < end)
    {
        pollfd p{fd, POLLIN, 0};
        if (poll(&p, 1, 20) <= 0)
            continue;
        ssize_t n = recv(fd, buf, sizeof(buf), 0);
        if (n <= 0)
            break;
        out.append(buf, n);
    }
    return out;
}

std::string Framed(const std::string& body)
{
    uint32_t len = htonl(static_cast<uint32_t>(body.size()));
    return std::string(reinterpret_cast<const char*>(&len), 4) + body;
}

} // namespace

TEST(TwsProxyTest, Forwards)
{
    FakeServer server;
    TwsProxy proxy("127.0.0.1", server.port);
    int fd = ConnectLocal(proxy.Port());
    ASSERT_GE(fd, 0);
    EXPECT_EQ(Echo(fd, "hello"), "hello");
    EXPECT_EQ(proxy.OpenConnections(), 1);
    close(fd);
}

TEST(TwsProxyTest, ResetAbortsTheConnection)
{
    FakeServer server;
    TwsProxy proxy("127.0.0.1", server.port);
    int fd = ConnectLocal(proxy.Port());
    ASSERT_GE(fd, 0);
    ASSERT_EQ(Echo(fd, "x"), "x");
    EXPECT_EQ(proxy.Reset(), 1);
    int err = -1;
    ReadToEnd(fd, err);
    EXPECT_EQ(err, ECONNRESET);
    EXPECT_EQ(proxy.OpenConnections(), 0);
    close(fd);
}

TEST(TwsProxyTest, CloseEndsTheStream)
{
    FakeServer server;
    TwsProxy proxy("127.0.0.1", server.port);
    int fd = ConnectLocal(proxy.Port());
    ASSERT_GE(fd, 0);
    ASSERT_EQ(Echo(fd, "x"), "x");
    EXPECT_EQ(proxy.Close(), 1);
    int err = -1;
    ReadToEnd(fd, err);
    EXPECT_EQ(err, 0);
    close(fd);
}

TEST(TwsProxyTest, RefuseThenAccept)
{
    FakeServer server;
    TwsProxy proxy("127.0.0.1", server.port);
    int port = proxy.Port();
    proxy.Refuse();
    EXPECT_EQ(ConnectLocal(port), -1);
    EXPECT_EQ(errno, ECONNREFUSED);
    proxy.Accept();
    EXPECT_EQ(proxy.Port(), port);
    int fd = ConnectLocal(port);
    ASSERT_GE(fd, 0);
    EXPECT_EQ(Echo(fd, "back"), "back");
    close(fd);
}

TEST(TwsProxyTest, CutAfterMessages)
{
    // the server sends three framed messages on connect; let only the first through
    FakeServer server(Framed("one") + Framed("two") + Framed("three"));
    TwsProxy proxy("127.0.0.1", server.port);
    proxy.CutNextAfter(1);
    int fd = ConnectLocal(proxy.Port());
    ASSERT_GE(fd, 0);
    int err = -1;
    EXPECT_EQ(ReadToEnd(fd, err), Framed("one"));
    EXPECT_EQ(err, 0);
    close(fd);

    // only the next connection is cut
    fd = ConnectLocal(proxy.Port());
    ASSERT_GE(fd, 0);
    std::string all = Framed("one") + Framed("two") + Framed("three");
    std::string got;
    auto end = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    char buf[256];
    while (got.size() < all.size() && std::chrono::steady_clock::now() < end)
    {
        pollfd p{fd, POLLIN, 0};
        if (poll(&p, 1, 20) <= 0)
            continue;
        ssize_t n = recv(fd, buf, sizeof(buf), 0);
        if (n <= 0)
            break;
        got.append(buf, n);
    }
    EXPECT_EQ(got, all);
    close(fd);
}

TEST(TwsProxyTest, CutAfterMessagesWithReset)
{
    FakeServer server(Framed("one") + Framed("two"));
    TwsProxy proxy("127.0.0.1", server.port);
    proxy.CutNextAfter(1, true);
    int fd = ConnectLocal(proxy.Port());
    ASSERT_GE(fd, 0);
    int err = -1;
    std::string got = ReadToEnd(fd, err);
    EXPECT_EQ(err, ECONNRESET);
    // data that arrived before the RST may be discarded, so only check nothing extra came through
    EXPECT_TRUE(got.empty() || got == Framed("one")) << got.size() << " bytes";
    close(fd);
}

TEST(TwsProxyTest, TwsDownClosesTheClient)
{
    int deadPort;
    {
        FakeServer gone;
        deadPort = gone.port;
    }
    TwsProxy proxy("127.0.0.1", deadPort);
    int fd = ConnectLocal(proxy.Port());
    ASSERT_GE(fd, 0);
    int err = -1;
    ReadToEnd(fd, err);
    EXPECT_TRUE(err == 0 || err == ECONNRESET) << "err " << err;
    close(fd);
}
#endif
