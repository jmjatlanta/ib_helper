#pragma once
#include "Contract.h"

#include <string>
#include <chrono>

struct hours
{
    tm open;
    tm close;
    tm start;
    tm stop;
};

class Exchange
{
    public:
    Exchange(const ContractDetails& contractDetails);

    void setStartTime(const std::string& in);
    void setStopTime(const std::string& in);
    bool isWithinRange(time_t in);

    time_t premarketStart(time_t today);
    time_t marketOpen(time_t today);
    time_t marketClose(time_t today);
    time_t marketStart(time_t today);
    time_t marketStop(time_t today);

    private:
    std::string timeZone;
    std::string liquidHours;
    hours exchangeHours;

    private:
    std::chrono::time_point<std::chrono::system_clock> midnightAtExchange(time_t today);
    time_t calculateFromTm(time_t today, tm hour);
};
