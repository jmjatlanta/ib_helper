#pragma once
#include <string>
#include "Contract.h"

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
};

} // end namespace ib_helper