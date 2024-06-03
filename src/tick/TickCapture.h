#pragma once
#include "Contract.h"
#include "IBConnector.hpp"

/***
 * capture ticks into a tick database
*/
class TickCapture
{
    public:
    TickCapture(const Contract& contract, std::shared_ptr<ib_helper::IBConnector> conn);

private:
};:
