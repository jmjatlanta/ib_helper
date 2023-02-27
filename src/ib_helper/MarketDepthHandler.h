#pragma once

#include <string>

class MarketDepthHandler 
{
    public:
	virtual void OnUpdateMktDepth(int tickerId, int position, int operation, int side, double price, Decimal size) = 0; 
	virtual void OnUpdateMktDepthL2(int tickerId, int position, const std::string& marketMaker, int operation, int side, 
            double price, Decimal size, boolean isSmartDepth) = 0;
};
