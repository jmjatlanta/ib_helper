#pragma once
#include <string>

namespace ib_helper
{

class IBConnector;

class IBConnectionMonitor
{
    public:
    /****
     * We've just connected to IB
    */
    virtual void OnConnect(IBConnector* conn) = 0;
    /****
     * We've connected to IB and it has replied with some basic information
     * (nextValidId() was called)
    */
    virtual void OnFullConnect(IBConnector* conn) = 0;
    /*****
     * We have been disconnected from IB
    */
    virtual void OnDisconnect(IBConnector* conn) = 0;
    /****
     * IB has reported an error
     * @param conn the connection
     * @param id the order/subscription id that is related to the error (or -1)
     * @param code the error code
     * @param msg the text of the error
     * @param json advanced order error information in json format
    */
    virtual void OnError(IBConnector* conn, int id, int errorCode, const std::string& msg, const std::string& jsonj) = 0;
    /****
     * The connection between TWS and IB's servers changed state
     * (1100: up = false; 1101: up = true, dataLost = true; 1102: up = true, dataLost = false).
     * Called in addition to OnError.
     * @param conn the connection
     * @param up true if TWS is connected to IB again
     * @param dataLost true if subscriptions were lost and must be re-requested
    */
    virtual void OnIbConnectivity(IBConnector* conn, bool up, bool dataLost) {}
};

} // namespace ib_helper

