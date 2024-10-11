#pragma once
#include <string>

namespace ib_helper 
{

class DisplayGroupHandler
{
    public:
    virtual void OnDisplayGroupUpdated(int reqId, const std::string& contractInfo) = 0;
};

}

