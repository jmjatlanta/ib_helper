#pragma once
#include "IBConnector.hpp"
#include "Logger.h"

namespace ib_helper
{

/****
 * A connection to IB with automatic reconnects
*/
class IBManagedConnector : public IBConnector
{
public:
    IBManagedConnector(const std::string& host, int port, int clientId, IBConnectionMonitor* monitor);
    ~IBManagedConnector();

    /***
     * perform the base disconnect() and possibly start async_connect
     */
    void connectionClosed() override;

    /***
     *  continually try to connect until success, or no connection listeners
     *  should be run on a separate thread
    */
    void async_reconnect();

protected:
    void cleanUpPartialConnection() override;

protected:
    std::thread reconnectThread;
    std::atomic<bool> shouldAttemptConnect = false;
    std::atomic<bool> currentlyAttempting = false;
    Logger* logger = nullptr;
};


} // namespace ib_helper
