#pragma once
#include "../ib_api/client/Decimal.h"
#include "../ib_api/client/Contract.h"
#include <string>

class Position
{
    public:
    Position(const std::string& account, Contract contract, Decimal pos, double avgCost)
            : account(account), contract(contract), pos(pos), avgCost(avgCost) {}

    public:
    std::string account;
    Contract contract;
    Decimal pos = 0;
    double avgCost = 0.0;    
};

