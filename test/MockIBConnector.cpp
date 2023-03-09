#include "MockIBConnector.h"
#include "../src/util/SysLogger.h"
#include <stdexcept>
#include <exception>

MockIBConnector::MockIBConnector(const std::string& hostname, int port, int clientId) 
    : nextRequestId(1), nextOrderId(1)
{
}

MockIBConnector::~MockIBConnector() {}

uint32_t MockIBConnector::GetNextRequestId()
{
    return ++nextRequestId;
}

void doGetContractDetails(const Contract& contract, std::promise<ContractDetails>& promise)
{
    if (contract.symbol == "ES" && contract.secType == "FUT")
    {
        ContractDetails dets;
        dets.contract = contract;
        promise.set_value(dets);
    }
    if (contract.symbol == "XYZABC")
    {
        try
        {
            promise.set_exception(std::make_exception_ptr( std::out_of_range("Invalid symbol: " + contract.symbol ))); 
        } catch (...)
        {
            // set_exception can throw
            util::SysLogger::getInstance()->error("set_exception threw");
        }
    }
}

std::future<ContractDetails> MockIBConnector::GetContractDetails(const Contract& contract)
{
    // we have some that are good and some that are bad
    uint32_t promiseId = GetNextRequestId();
    auto& promise = contractDetailsHandlers[promiseId];
    doGetContractDetails(contract, promise);
    return promise.get_future();
}

bool MockIBConnector::IsConnected() const { return true; }

void MockIBConnector::RequestPositions()
{
    positionEnd();
}

void MockIBConnector::RequestOpenOrders()
{
    openOrderEnd();
}
