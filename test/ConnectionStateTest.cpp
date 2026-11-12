#include "../src/ib_helper/IBConnector.hpp"
#include <gtest/gtest.h>
#include <vector>
#include <atomic>
#include <future>
#include <memory>
#include <thread>
#include "EReaderOSSignal.h"
#ifndef _WIN32
#include "TwsProxy.h"
#endif

/***
 * Tests that a socket failure reported through error() (502/509) updates the
 * connection state and tells the monitors, without disconnecting from inside
 * the TWS callback. None of these need a running TWS.
 */

namespace {

using ib_helper::IBConnector;

class StateConnector : public IBConnector
{
    public:
    StateConnector() : IBConnector() {}
    using IBConnector::error;
    void SetStatus(ConnectionStatus in) { currentConnectionStatus = in; }
    // disconnect() joins the reader thread, so it must not be called from error()
    bool disconnect() override { ++disconnectCalls; return true; }
    // IBManagedConnector reconnects from its connectionClosed() override
    void connectionClosed() override { ++connectionClosedCalls; IBConnector::connectionClosed(); }
    void cleanUpPartialConnection() override { ++cleanUpCalls; }
    int disconnectCalls = 0;
    int connectionClosedCalls = 0;
    int cleanUpCalls = 0;
};

class RecordingMonitor : public ib_helper::IBConnectionMonitor
{
    public:
    void OnConnect(IBConnector*) override {}
    void OnFullConnect(IBConnector*) override {}
    void OnDisconnect(IBConnector*) override { ++disconnects; }
    void OnError(IBConnector*, int, int code, const std::string&, const std::string&) override { codes.push_back(code); }
    int disconnects = 0;
    std::vector<int> codes;
};

} // namespace

class ConnectionStateTest : public ::testing::TestWithParam<int> {};

TEST_P(ConnectionStateTest, SocketFailureWhileConnected)
{
    const int code = GetParam();
    StateConnector conn;
    RecordingMonitor monitor;
    conn.AddConnectionMonitor(&monitor);
    conn.SetStatus(IBConnector::ConnectionStatus::FULLY_CONNECTED);

    conn.error(-1, 0, code, "Bad Message Length / socket failure", "");

    EXPECT_EQ(conn.disconnectCalls, 0);
    EXPECT_EQ(conn.connectionClosedCalls, 1);
    EXPECT_EQ(conn.GetConnectionStatus(), IBConnector::ConnectionStatus::SHUTDOWN);
    EXPECT_FALSE(conn.IsConnected());
    EXPECT_EQ(monitor.codes, std::vector<int>{code});
    EXPECT_EQ(monitor.disconnects, 1);
    conn.RemoveConnectionMonitor(&monitor);
}

TEST_P(ConnectionStateTest, SocketFailureWhilePartiallyConnected)
{
    StateConnector conn;
    RecordingMonitor monitor;
    conn.AddConnectionMonitor(&monitor);
    conn.SetStatus(IBConnector::ConnectionStatus::PARTIALLY_CONNECTED);

    conn.error(-1, 0, GetParam(), "Couldn't connect to TWS", "");

    EXPECT_EQ(conn.disconnectCalls, 0);
    EXPECT_EQ(conn.connectionClosedCalls, 0);
    EXPECT_EQ(conn.cleanUpCalls, 1);
    EXPECT_EQ(monitor.codes.size(), 1u);
    EXPECT_EQ(monitor.disconnects, 0);
    conn.RemoveConnectionMonitor(&monitor);
}

TEST_P(ConnectionStateTest, SocketFailureWhenAlreadyDown)
{
    // e.g. a send fails after the connection already closed: nothing more to do
    StateConnector conn;
    RecordingMonitor monitor;
    conn.AddConnectionMonitor(&monitor);
    conn.SetStatus(IBConnector::ConnectionStatus::SHUTDOWN);

    conn.error(-1, 0, GetParam(), "Couldn't connect to TWS", "");

    EXPECT_EQ(conn.disconnectCalls, 0);
    EXPECT_EQ(conn.connectionClosedCalls, 0);
    EXPECT_EQ(conn.cleanUpCalls, 0);
    EXPECT_EQ(monitor.disconnects, 0);
    EXPECT_EQ(conn.GetConnectionStatus(), IBConnector::ConnectionStatus::SHUTDOWN);
    conn.RemoveConnectionMonitor(&monitor);
}

INSTANTIATE_TEST_SUITE_P(SocketCodes, ConnectionStateTest, ::testing::Values(502, 509));

namespace {

/*** uses the real disconnect(); with no TWS client or reader thread it only changes state and notifies */
class PlainConnector : public IBConnector
{
    public:
    PlainConnector() : IBConnector() {}
    using IBConnector::error;
    using IBConnector::connectionClosed;
    void SetStatus(ConnectionStatus in) { currentConnectionStatus = in; }
};

} // namespace

TEST(DisconnectNotifyTest, DisconnectWhileConnectedNotifiesOnce)
{
    PlainConnector conn;
    RecordingMonitor monitor;
    conn.AddConnectionMonitor(&monitor);
    conn.SetStatus(IBConnector::ConnectionStatus::FULLY_CONNECTED);
    EXPECT_TRUE(conn.disconnect());
    EXPECT_EQ(conn.GetConnectionStatus(), IBConnector::ConnectionStatus::SHUTDOWN);
    EXPECT_EQ(monitor.disconnects, 1);
    // a second disconnect() has nothing left to close
    EXPECT_TRUE(conn.disconnect());
    EXPECT_EQ(monitor.disconnects, 1);
    conn.RemoveConnectionMonitor(&monitor);
}

TEST(DisconnectNotifyTest, CleanupAfterDropDoesNotNotifyAgain)
{
    for(int code : {502, 509})
    {
        PlainConnector conn;
        RecordingMonitor monitor;
        conn.AddConnectionMonitor(&monitor);
        conn.SetStatus(IBConnector::ConnectionStatus::FULLY_CONNECTED);
        conn.error(-1, 0, code, "socket failure", "");
        EXPECT_EQ(monitor.disconnects, 1) << "code " << code;
        conn.disconnect(); // the owner cleans up
        EXPECT_EQ(monitor.disconnects, 1) << "code " << code;
        conn.RemoveConnectionMonitor(&monitor);
    }
}

TEST(DisconnectNotifyTest, CleanupAfterClosedConnectionDoesNotNotifyAgain)
{
    PlainConnector conn;
    RecordingMonitor monitor;
    conn.AddConnectionMonitor(&monitor);
    conn.SetStatus(IBConnector::ConnectionStatus::FULLY_CONNECTED);
    conn.connectionClosed(); // TWS closed the connection
    EXPECT_EQ(monitor.disconnects, 1);
    conn.disconnect();
    EXPECT_EQ(monitor.disconnects, 1);
    conn.RemoveConnectionMonitor(&monitor);
}

TEST(DisconnectNotifyTest, DisconnectWhenNeverConnectedDoesNotNotify)
{
    PlainConnector conn;
    RecordingMonitor monitor;
    conn.AddConnectionMonitor(&monitor);
    EXPECT_EQ(conn.GetConnectionStatus(), IBConnector::ConnectionStatus::NOT_STARTED);
    EXPECT_TRUE(conn.disconnect());
    EXPECT_EQ(monitor.disconnects, 0);
    EXPECT_EQ(conn.GetConnectionStatus(), IBConnector::ConnectionStatus::SHUTDOWN);
    conn.RemoveConnectionMonitor(&monitor);
}

TEST(DisconnectNotifyTest, DisconnectWhilePartiallyConnectedNotifies)
{
    PlainConnector conn;
    RecordingMonitor monitor;
    conn.AddConnectionMonitor(&monitor);
    conn.SetStatus(IBConnector::ConnectionStatus::PARTIALLY_CONNECTED);
    conn.disconnect();
    EXPECT_EQ(monitor.disconnects, 1);
    conn.RemoveConnectionMonitor(&monitor);
}

#ifndef _WIN32
namespace {

std::atomic<int> deletedClients = 0;
std::atomic<int> deletedSignals = 0;

class TrackedClient : public EClientSocket
{
    public:
    TrackedClient() : EClientSocket(nullptr, nullptr) {}
    ~TrackedClient() override { ++deletedClients; }
};

class TrackedSignal : public EReaderOSSignal
{
    public:
    TrackedSignal() : EReaderOSSignal(10) {}
    ~TrackedSignal() override { ++deletedSignals; }
};

/*** a connector left as a dropped connection leaves it: SHUTDOWN, with the old client still allocated */
class DroppedConnector : public IBConnector
{
    public:
    explicit DroppedConnector(int port) : IBConnector()
    {
        hostname = "127.0.0.1";
        this->port = port;
        clientId = 910;
        currentConnectionStatus = ConnectionStatus::SHUTDOWN;
        osSignal = new TrackedSignal();
        ibClient = new TrackedClient();
    }
    void SetListener(std::shared_ptr<std::thread> in) { listenerThread = in; }
    bool HasListener() const { return listenerThread != nullptr; }
    bool HasClient() const { return ibClient != nullptr || osSignal != nullptr; }
    void JoinListener() { if (listenerThread != nullptr && listenerThread->joinable()) listenerThread->join(); }
};

} // namespace

TEST(ConnectCleanupTest, ConnectReleasesWhatADropLeftBehind)
{
    // nothing listens on this port, so the new connection fails after the cleanup
    TwsProxy proxy("127.0.0.1", 1);
    proxy.Refuse();
    deletedClients = 0;
    deletedSignals = 0;
    DroppedConnector conn(proxy.Port());
    std::atomic<bool> oldReaderRan = false;
    conn.SetListener(std::make_shared<std::thread>([&]{ oldReaderRan = true; }));

    EXPECT_FALSE(conn.connect());
    EXPECT_TRUE(oldReaderRan);
    EXPECT_EQ(deletedClients, 1);
    EXPECT_EQ(deletedSignals, 1);
    EXPECT_FALSE(conn.HasListener());
    EXPECT_FALSE(conn.HasClient());
    EXPECT_EQ(conn.GetConnectionStatus(), IBConnector::ConnectionStatus::NOT_STARTED);
}

TEST(ConnectCleanupTest, ConnectFromTheReaderThreadIsRefused)
{
    TwsProxy proxy("127.0.0.1", 1);
    proxy.Refuse();
    deletedClients = 0;
    DroppedConnector conn(proxy.Port());
    // run connect() on the thread the connector thinks is its reader thread, as a TWS callback would
    std::promise<void> go;
    auto started = go.get_future().share();
    std::atomic<int> result = -1;
    conn.SetListener(std::make_shared<std::thread>([&, started]{
        started.wait();
        result = conn.connect() ? 1 : 0;
    }));
    go.set_value();
    conn.JoinListener();

    EXPECT_EQ(result, 0);
    EXPECT_EQ(deletedClients, 0); // nothing released from under the "callback"
    EXPECT_TRUE(conn.HasClient());
    EXPECT_EQ(conn.GetConnectionStatus(), IBConnector::ConnectionStatus::SHUTDOWN);
    // a later connect() from another thread cleans up
    EXPECT_FALSE(conn.connect());
    EXPECT_EQ(deletedClients, 1);
}
#endif
