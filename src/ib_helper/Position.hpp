#pragma once
#include "../ib_api/client/Decimal.h"
#include "../ib_api/client/Contract.h"
#include <string>

namespace ib_helper
{

class Position
{
    public:
    Position(const std::string& account, Contract contract, Decimal pos, double avgCost)
            : account(account), contract(contract), pos(pos), averageCost(avgCost), expectedPos(pos) {}
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

