#include "Contract.h"
#include "Decimal.h"

#include <string>

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
	String accountName;

	Position(Contract contract, Decimal position, double marketPrice, double marketValue,
			double averageCost, double unrealizedPNL, double realizedPNL, const std::string& accountName)
            : contract(contract), position(position), marketPrice(marketPrice), marketValue(marketValue),
            averageCost(averageCost), unrealizedPNL(unrealizedPNL), realizedPNL(realizedPNL),
            accountName(accountName)
    {
	}
	
	Position(Contract contract, const std::string& accountName) 
         : Position(contract, Decimal.get(0), 0.0, 0.0, 0.0, 0.0, 0.0, accountName)
	{
	}
	
	Decimal getPosition() { return position; }
	
	double getAverageCost()
	{
		double multiplier = 1;
		try
		{
			if (contract.multiplier() != null)
				multiplier = Double.parseDouble(contract.multiplier());
			if (multiplier == 0.0)
				multiplier = 1;
		} catch (const std::exception& e) {}

		return averageCost / multiplier;
	}
}

