#pragma once
#include "Contract.h"
#include "Decimal.h"
#include "Position.h"
#include "Order.h"

#include <string>
#include <exception>

class AccountHandler
{
    public:
    virtual void OnAccountValueUpdate(const std::string& key, const std::string& value, const std::string& currency,
            const std::string& accountName) = 0;
    virtual void OnPortfolioUpdate(const Contract& contract, Decimal position, double marketPrice, double marketValue, 
			double averageCost, double unrealizedPNL, double realizedPNL, const std::string& accountName);
	virtual void OnUpdateAccountTime(const std::string& timestamp);
	virtual void OnAccountDownloadEnd(const std::string& accountName);
	virtual void OnPosition(const std::string& account, const Contract& contract, Decimal pos, double avgCost);
	virtual void OnPositionEnd();
	virtual bool isApproved(const Contract& contract, const Order& order);
	virtual void OnCurrentTime(long currentTime);
	virtual Position GetPosition(const Contract& contract, const std::string& account);
	virtual void OnError(const std::exception& e);
	virtual void OnError(const std::string& msg);
	virtual void OnError(int id, int errorCode, const std::string& errorMsg, const std::string& advancedOrderRejectJson);
};

