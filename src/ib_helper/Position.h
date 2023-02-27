#pragma once
#include "Contract.h"
#include "Decimal.h"

#include <string>

namespace ib_helper
{

class Position 
{
    public:
	Contract contract;
	Decimal position;
	double marketPrice = 0.0;
	double marketValue = 0.0;
	double averageCost = 0.0;;
	double unrealizedPNL = 0.0;
	double realizedPNL = 0.0;
	const std::string& accountName;

	Position(Contract contract, Decimal position, double marketPrice, double marketValue,
			double averageCost, double unrealizedPNL, double realizedPNL, const std::string& accountName)
            : contract(contract), position(position), marketPrice(marketPrice), marketValue(marketValue),
            averageCost(averageCost), unrealizedPNL(unrealizedPNL), realizedPNL(realizedPNL),
            accountName(accountName)
    {
	}
	
	Position(Contract contract, const std::string& accountName) 
         : Position(contract, doubleToDecimal(0.0), 0.0, 0.0, 0.0, 0.0, 0.0, accountName)
	{
	}
	
	Decimal getPosition() { return position; }
	
	double getAverageCost()
	{
		double multiplier = 1;
		if (!contract.multiplier.empty())
			multiplier = strtod(contract.multiplier.c_str(), nullptr);
        if (multiplier == 0.0)
			multiplier = 1;

		return averageCost / multiplier;
	}
};

}

