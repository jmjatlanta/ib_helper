#pragma once
#include "DecimalHelper.hpp"
#include "Contract.h"
#include <string>

namespace ib_helper
{

class Position
{
    public:

    Position(const Contract& contract, Decimal pos, double avgCost, double unrealizedPNL,
            double realizedPNL, const std::string& account)
            : contract(contract), pos(pos), averageCost(avgCost), expectedPos(pos),
            unrealizedPNL(unrealizedPNL), realizedPNL(realizedPNL), account(account)
    {}

    Position() : account(""), averageCost(0) {}

    Position(const Contract& contract, const std::string& accountName)
        : contract(contract), account(accountName) {}

    Decimal GetSize() const { return expectedPos; }
    bool InSync() const { return expectedPos == pos; }

    public:
    std::string account;
    Contract contract;
    Decimal pos = doubleToDecimal(0.0);
    double averageCost = 0.0;    
    double unrealizedPNL = 0.0;
    double realizedPNL = 0.0;
    Decimal expectedPos = 0;
};

} // namespace ib_helper

