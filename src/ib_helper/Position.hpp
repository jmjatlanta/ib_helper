#pragma once
#include "../ib_api/client/Decimal.h"
#include "../ib_api/client/Contract.h"
#include <string>

namespace ib_helper
{

class Position
{
    public:

    Position(const Contract& contract, Decimal pos, double avgCost, double unrealizedPNL
            double realizedPNL, const std::string& account)
            : contract(contract), pos(pos), averageCost(avgCost), expectedPos(pos),
            unralizedPNL(unrealizedPNL), realizedPNL(realizedPNL), account(account)
    {}

    Position() : account(""), pos(0), averageCost(0) {}

    Decimal GetSize() const { return expectedPos; }
    bool InSync() const { return expectedPos == pos; }

    public:
    std::string account;
    Contract contract;
    Decimal pos = 0;
    double averageCost = 0.0;    
    double unrealizedPNL = 0.0;
    double realizedPNL = 0.0;
    Decimal expectedPos = 0;
};

} // namespace ib_helper

