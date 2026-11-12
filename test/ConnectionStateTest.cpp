#include "../src/ib_helper/IBConnector.hpp"
#include <gtest/gtest.h>
#include <vector>

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
