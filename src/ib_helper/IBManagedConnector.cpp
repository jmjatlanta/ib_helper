#include "IBManagedConnector.hpp"

namespace ib_helper
{

IBManagedConnector::IBManagedConnector(const std::string& host, int port, int clientId, IBConnectionMonitor* monitor)
        : IBConnector(host, port, clientId, monitor), logger(Logger::getInstance())
{
    shouldAttemptConnect = true;
    logger->debug("IBManagedConnector", "ctor complete");
}

IBManagedConnector::~IBManagedConnector()
{
    logger->debug("IBManagedConnector", "dtor called");
    shouldAttemptConnect = false;
    if (reconnectThread.joinable())
        reconnectThread.join();
    logger->debug("IBManagedConnector", "dtor complete");
}

void IBManagedConnector::async_connect()
{
    logger->debug("IBManagedConnector", "async_connect called");
    if (reconnectThread.joinable())
        reconnectThread.join();
    reconnectThread = std::thread( [this]()
            {
                logger->debug("IBManagedConnector", "async_connect thread started");
                // attempt to change to true... if already true, exit
                bool expected = false;
                if (currentlyAttempting.compare_exchange_strong(expected, true))
                {
                    try
                    {
                        while (shouldAttemptConnect && !connect())
                            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                    } 
                    catch(const std::exception& ex)
                    {
                        logger->error("IBManagedConnector", "async_connect thread threw " + std::string(ex.what()));
                    }
                    catch(...)
                    {
                        logger->error("IBManagedConnector", "async_connect thread threw.");
                    }
                    currentlyAttempting = false;
                }
                logger->debug("IBManagedConnector", "async_connect thread completed");
            });
}

void IBManagedConnector::connectionClosed()
{
    logger->debug("IBManagedConnector", "connectionClosed called");
    IBConnector::connectionClosed();
    if (shouldAttemptConnect && !currentlyAttempting)
        async_connect();
    logger->debug("IBManagedConnector", "connectionClosed completed");
}

} // namespace ib_helper
