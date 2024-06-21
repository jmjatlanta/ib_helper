#include "TickCapture.h"

#include <sstream>
#include <iostream>

std::string to_string(const AllLast& in)
{
    std::stringstream ss;
    ss << in.time << "," 
        << in.price << "," 
        << in.size;
    return ss.str();
}

TickCapture::TickCapture(const Contract& contract, std::shared_ptr<ib_helper::IBConnector> conn, std::ofstream& out)
    : contract(contract), conn(conn), out( out )
{
    if (conn == nullptr)
        throw std::invalid_argument("IBConnector cannot be nullptr");
    reqId = conn->SubscribeToTickByTick(contract, this, "AllLast", 0, true );
}

/***
 * We received a tick, we should store it
 * @param reqId the request id (make sure it is ours)
 * @param tickType "Last" or "AllLast"
 * @param time the time
 * @param price
 * @param size
 * @param tickAttribLast
 * @param exchange
 * @param specialConditions
 */
void TickCapture::OnTickByTickAllLast(int reqId, int tickType, time_t time, double price, Decimal size,
        const TickAttribLast& tickAttribLast, const std::string& exchange, 
        const std::string& specialConditions)
{
    if (reqId == this->reqId)
    {
        AllLast curr{time, price, static_cast<uint64_t>(decimalToDouble(size))};
        out.write( (const char*)&curr, sizeof(time_t) + sizeof(double) + sizeof(uint64_t));
    }
}

AllLast TickReader::read()
{
    AllLast curr;
    in.read( (char*)&curr, sizeof(time_t) + sizeof(double) + sizeof(uint64_t));
    return curr;
};
