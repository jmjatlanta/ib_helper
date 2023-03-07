#pragma once
#include "../ib_api/client/Contract.h"

#include <string>
#include <chrono>

struct hours
{
    tm open;
    tm close;
};

class Exchange
{
    public:
    Exchange(const ContractDetails& contractDetails);

    time_t premarketStart(time_t today = 0);
    time_t marketOpen(time_t today = 0);
    time_t marketClose(time_t today = 0);

    private:
    std::string timeZone;
    std::string liquidHours;
    hours exchangeHours;

    private:
    std::chrono::time_point<std::chrono::system_clock> midnightAtExchange(time_t today);
};
