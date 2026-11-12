#pragma once
#include <string>
#include "Contract.h"
#include "IbError.hpp"

namespace ib_helper 
{

class IBConnector;

class ScannerHandler
{
    public:
    virtual void OnScannerParameters(ib_helper::IBConnector* conn, const std::string& xml) = 0;
    virtual void OnScannerData(ib_helper::IBConnector* conn, int reqId, int rank, const ContractDetails& contractDetails, const std::string& distance,
            const std::string& benchmark, const std::string& projection, const std::string& legsStr) = 0;
    virtual void OnScannerDataEnd(ib_helper::IBConnector* conn, int reqId) = 0;
    virtual void OnScannerSubscriptionEnd(ib_helper::IBConnector* conn, int reqId) = 0;
    /***
     * TWS reported an error or message for a scanner subscription
     */
    virtual void OnError(ib_helper::IBConnector* conn, int reqId, const IbError& err) {}
};

} // end namespace ib_helper