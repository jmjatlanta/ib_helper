#pragma once
#include "../ib_api/client/Contract.h"

#include <string>

namespace ib_helper 
{

/***
* Implement to receive account level updates
*/

class AccountHandler
{
    public:
    virtual void OnAccountValueUpdate(const std::string& key, const std::string& value, const std::string& currency,
            const std::string& accountName) = 0;
    virtual void OnPortfolioUpdate(const Contract& contract, Decimal position, double marketPrice, double marketValue,
            double averageCost, double unrealizedPNL, double realizedPNL, const std::string& accountName) = 0;
    virtual void OnUpdateAccountTime(const std::string& timestamp) = 0;
    virtual void OnAccountDownloadEnd(const std::string& accountName) = 0;
    virtual void OnPosition(const std::string& account, const Contract& contract, Decimal pos, double averageCost) = 0;
    virtual void OnPositionEnd() = 0;
    virtual void OnCurrentTime(long currentTime) = 0;
    virtual void OnError(int id, int errorCode, const std::string& errorMessage, 
            const std::string& advancedOrderRejectJson) = 0;
};

} // end namespace ib_helper
