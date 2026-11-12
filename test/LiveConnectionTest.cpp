#ifndef _WIN32
#include "../src/ib_helper/IBConnector.hpp"
#include "../src/ib_helper/IBManagedConnector.hpp"
#include "TwsProxy.h"
#include <gtest/gtest.h>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

/***
 * connect()/disconnect() against a running TWS, directly and through TwsProxy,
 * which breaks the connection on command.
 * Skipped unless IB_LIVE_TEST is set. IB_TWS_HOST / IB_TWS_PORT (default 127.0.0.1:7497)
 * say where TWS is. Uses client ids 901-909. Places no orders.
 */

namespace {

using ib_helper::IBConnector;
using Status = IBConnector::ConnectionStatus;
using namespace std::chrono_literals;

std::string TwsHost()
{
    const char* v = std::getenv("IB_TWS_HOST");
    return v == nullptr ? "127.0.0.1" : v;
}
int TwsPort()
{
    const char* v = std::getenv("IB_TWS_PORT");
    return v == nullptr ? 7497 : std::atoi(v);
}

bool WaitFor(std::function<bool()> pred, std::chrono::milliseconds timeout)
{
    auto end = std::chrono::steady_clock::now() + timeout;
    while (std::chrono::steady_clock::now() < end)
    {
        if (pred())
            return true;
        std::this_thread::sleep_for(20ms);
    }
    return pred();
}

class Monitor : public ib_helper::IBConnectionMonitor
{
    public:
    void OnConnect(IBConnector*) override { ++connects; }
    void OnFullConnect(IBConnector*) override { ++fullConnects; }
    void OnDisconnect(IBConnector*) override { ++disconnects; }
    void OnError(IBConnector*, int, int code, const std::string&, const std::string&) override
    {
        std::scoped_lock lock(m);
        codes.push_back(code);
    }
    bool Saw(int code)
    {
        std::scoped_lock lock(m);
        return std::find(codes.begin(), codes.end(), code) != codes.end();
    }
    std::string Codes()
    {
        std::scoped_lock lock(m);
        std::string out;
        for(int c : codes)
            out += std::to_string(c) + " ";
        return out;
    }
    std::atomic<int> connects = 0;
    std::atomic<int> fullConnects = 0;
    std::atomic<int> disconnects = 0;
    std::mutex m;
    std::vector<int> codes;
};

/*** exposes the client pointer so tests can see what connect() does with the old one */
class Connector : public IBConnector
{
    public:
    Connector(const std::string& host, int port, int clientId, ib_helper::IBConnectionMonitor* mon)
            : IBConnector(host, port, clientId, mon) {}
    const void* Client() const { return ibClient; }
};

class LiveConnectionTest : public ::testing::Test
{
    protected:
    void SetUp() override
    {
        if (std::getenv("IB_LIVE_TEST") == nullptr)
            GTEST_SKIP() << "set IB_LIVE_TEST to run against TWS";
        proxy = std::make_unique<TwsProxy>(TwsHost(), TwsPort());
    }
    int ProxyPort() const { return proxy->Port(); }
    std::unique_ptr<TwsProxy> proxy;
};

/*** break a working connection through the proxy, then disconnect() and connect() from this thread */
void DropThenRecover(TwsProxy& proxy, bool reset, int clientId)
{
    const char* how = reset ? "reset" : "close";
    Monitor mon;
    Connector conn("127.0.0.1", proxy.Port(), clientId, &mon);
    ASSERT_TRUE(WaitFor([&]{ return conn.IsConnected(); }, 5s));
    ASSERT_EQ(reset ? proxy.Reset() : proxy.Close(), 1);

    ASSERT_TRUE(WaitFor([&]{ return mon.disconnects >= 1; }, 5s))
            << "no OnDisconnect after " << how << "; status " << to_string(conn.GetConnectionStatus())
            << " codes: " << mon.Codes();
    EXPECT_EQ(conn.GetConnectionStatus(), Status::SHUTDOWN);
    EXPECT_FALSE(conn.IsConnected());
    if (reset)
        EXPECT_TRUE(mon.Saw(509)) << "codes: " << mon.Codes();

    // the owner cleans up from its own thread; this must not hang, nor report the drop again
    EXPECT_TRUE(conn.disconnect());
    EXPECT_EQ(conn.Client(), nullptr);
    EXPECT_EQ(mon.disconnects, 1);

    ASSERT_TRUE(conn.connect());
    EXPECT_TRUE(WaitFor([&]{ return conn.IsConnected(); }, 5s)) << "status " << to_string(conn.GetConnectionStatus());
    conn.disconnect();
}

} // namespace

TEST_F(LiveConnectionTest, DirectConnectAndDisconnect)
{
    Monitor mon;
    Connector conn(TwsHost(), TwsPort(), 901, &mon);
    ASSERT_TRUE(WaitFor([&]{ return conn.IsConnected(); }, 5s)) << "status " << to_string(conn.GetConnectionStatus());
    EXPECT_TRUE(WaitFor([&]{ return mon.fullConnects >= 1; }, 2s));

    EXPECT_TRUE(conn.disconnect());
    EXPECT_EQ(conn.GetConnectionStatus(), Status::SHUTDOWN);
    EXPECT_EQ(mon.disconnects, 1);
    EXPECT_EQ(conn.Client(), nullptr);
}

TEST_F(LiveConnectionTest, RepeatedDisconnectConnect)
{
    Monitor mon;
    Connector conn(TwsHost(), TwsPort(), 902, &mon);
    ASSERT_TRUE(WaitFor([&]{ return conn.IsConnected(); }, 5s));
    for(int i = 0; i < 5; ++i)
    {
        ASSERT_TRUE(conn.disconnect());
        ASSERT_EQ(conn.GetConnectionStatus(), Status::SHUTDOWN);
        ASSERT_TRUE(conn.connect()) << "cycle " << i;
        ASSERT_TRUE(WaitFor([&]{ return conn.IsConnected(); }, 5s)) << "cycle " << i
                << " status " << to_string(conn.GetConnectionStatus());
    }
    EXPECT_EQ(mon.fullConnects, 6);
    EXPECT_EQ(mon.disconnects, 5);
    conn.disconnect();
}

TEST_F(LiveConnectionTest, RefusedThenAccepted)
{
    proxy->Refuse();
    Monitor mon;
    Connector conn("127.0.0.1", ProxyPort(), 903, &mon); // constructor's connect() fails
    EXPECT_EQ(conn.GetConnectionStatus(), Status::NOT_STARTED);
    EXPECT_TRUE(mon.Saw(502)) << "codes: " << mon.Codes();
    EXPECT_EQ(conn.Client(), nullptr);

    proxy->Accept();
    ASSERT_TRUE(conn.connect());
    EXPECT_TRUE(WaitFor([&]{ return conn.IsConnected(); }, 5s)) << "status " << to_string(conn.GetConnectionStatus());
    conn.disconnect();
}

TEST_F(LiveConnectionTest, ResetWhileConnected)
{
    DropThenRecover(*proxy, true, 904);
}

TEST_F(LiveConnectionTest, CloseWhileConnected)
{
    DropThenRecover(*proxy, false, 905);
}

TEST_F(LiveConnectionTest, ManagedReconnectsAfterReset)
{
    Monitor mon;
    auto conn = std::make_unique<ib_helper::IBManagedConnector>("127.0.0.1", ProxyPort(), 906, &mon);
    ASSERT_TRUE(WaitFor([&]{ return conn->IsConnected(); }, 5s));
    ASSERT_EQ(mon.fullConnects, 1);
    ASSERT_EQ(proxy->Reset(), 1);

    ASSERT_TRUE(WaitFor([&]{ return mon.disconnects >= 1; }, 5s)) << "codes: " << mon.Codes();
    EXPECT_TRUE(WaitFor([&]{ return mon.fullConnects >= 2 && conn->IsConnected(); }, 15s))
            << "no reconnect; status " << to_string(conn->GetConnectionStatus()) << " codes: " << mon.Codes();
    conn->disconnect();
}

TEST_F(LiveConnectionTest, ManagedReconnectsWhileTwsDown)
{
    // the connection drops and TWS stays unreachable for a while
    Monitor mon;
    auto conn = std::make_unique<ib_helper::IBManagedConnector>("127.0.0.1", ProxyPort(), 907, &mon);
    ASSERT_TRUE(WaitFor([&]{ return conn->IsConnected(); }, 5s));
    proxy->Refuse();
    ASSERT_EQ(proxy->Reset(), 1);
    ASSERT_TRUE(WaitFor([&]{ return mon.disconnects >= 1; }, 5s));
    std::this_thread::sleep_for(3s); // several failed attempts
    EXPECT_FALSE(conn->IsConnected());
    EXPECT_TRUE(mon.Saw(502)) << "codes: " << mon.Codes();
    proxy->Accept();
    EXPECT_TRUE(WaitFor([&]{ return conn->IsConnected(); }, 15s))
            << "no reconnect; status " << to_string(conn->GetConnectionStatus()) << " codes: " << mon.Codes();
    conn->disconnect();
}

/*** the connection drops after the handshake but before nextValidId (PARTIALLY_CONNECTED) */
void HandshakeCut(TwsProxy& proxy, bool reset, int clientId)
{
    proxy.CutNextAfter(1, reset); // only the server-version reply gets through
    Monitor mon;
    auto conn = std::make_unique<ib_helper::IBManagedConnector>("127.0.0.1", proxy.Port(), clientId, &mon);
    EXPECT_TRUE(WaitFor([&]{ return conn->IsConnected(); }, 15s))
            << "no recovery after a cut handshake; status " << to_string(conn->GetConnectionStatus())
            << " codes: " << mon.Codes();
    // the first connection never got nextValidId; the reconnect is the only full connect
    EXPECT_EQ(mon.fullConnects, 1);
    if (reset)
        EXPECT_TRUE(mon.Saw(509)) << "the cut did not happen; codes: " << mon.Codes();
    else
        EXPECT_GE(mon.disconnects, 1) << "the cut did not happen";
    conn->disconnect();
}

TEST_F(LiveConnectionTest, ManagedHandshakeReset)
{
    HandshakeCut(*proxy, true, 908); // 509 while partially connected: cleanUpPartialConnection()
}

TEST_F(LiveConnectionTest, ManagedHandshakeClosed)
{
    HandshakeCut(*proxy, false, 908); // connectionClosed() while partially connected
}

TEST_F(LiveConnectionTest, ConnectAfterDropWithoutDisconnect)
{
    // connect() releases what the dropped connection left behind (the old reader thread,
    // ibClient and osSignal) before reconnecting; ConnectCleanupTest checks the release offline
    Monitor mon;
    Connector conn("127.0.0.1", ProxyPort(), 909, &mon);
    ASSERT_TRUE(WaitFor([&]{ return conn.IsConnected(); }, 5s));
    ASSERT_EQ(proxy->Reset(), 1);
    ASSERT_TRUE(WaitFor([&]{ return mon.disconnects >= 1; }, 5s));
    const void* oldClient = conn.Client();
    ASSERT_NE(oldClient, nullptr) << "the drop is expected to leave the client for the owner";
    ASSERT_TRUE(conn.connect());
    EXPECT_TRUE(WaitFor([&]{ return conn.IsConnected(); }, 5s)) << "status " << to_string(conn.GetConnectionStatus());
    EXPECT_EQ(mon.fullConnects, 2);
    EXPECT_EQ(mon.disconnects, 1);
    conn.disconnect();
}

TEST_F(LiveConnectionTest, OneOnDisconnectPerDrop)
{
    Monitor mon;
    auto conn = std::make_unique<ib_helper::IBManagedConnector>("127.0.0.1", ProxyPort(), 906, &mon);
    ASSERT_TRUE(WaitFor([&]{ return conn->IsConnected(); }, 5s));
    ASSERT_EQ(proxy->Reset(), 1);
    ASSERT_TRUE(WaitFor([&]{ return mon.fullConnects >= 2 && conn->IsConnected(); }, 15s));
    EXPECT_EQ(mon.disconnects, 1);
    conn.reset();
}
#endif
