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

/***
 * Parses IB's format of exchange hours
 */
class Exchange
{
    public:
    /**
     * @brief Construct a new Exchange object
     * 
     * @param contractDetails the contract
     */
    Exchange(const ContractDetails& contractDetails);

    /**
     * @brief determine if the given time is within the market open and close
     * 
     * @param in the time
     * @return true if "in" is between the exchange's open and close time
     */
    bool isWithinRange(time_t in);

    /***
     * @param today the date
     * @return the premarket start time
    */
    time_t premarketStart(time_t today);

    /**
     * @brief given a day, return the market open time
     * 
     * @param today a day (usually the current time)
     * @return the time of the market open
     */
    time_t marketOpen(time_t today);

    /**
     * @brief given a day, return the market close time
     * 
     * @param today a day (usually the current time)
     * @return the time of the market close
     */
    time_t marketClose(time_t today);

    /**
     * @brief given a day, return the market start time
     * 
     * @param today a day (usually the current time)
     * @return the time of the market start
     */
    time_t marketStart(time_t today);

    /**
     * @brief given a day, return the market stop time
     * 
     * @param today a day (usually the current time)
     * @return the time of the market stop 
     */
    time_t marketStop(time_t today);

    protected:
    /**
     * @brief adjust the exchange's liquid market start time
     * 
     * @param in the time in the format "HH:MM"
     */
    void setStartTime(const std::string& in);

    /**
     * @brief adjust the exchange's liquid market end time
     * 
     * @param in the time in the format "HH:MM"
     */
    void setStopTime(const std::string& in);

    /***
     * Determine the time_point of midnight for the date given
     * @param today the date
     * @return midnight of the given day
    */
    std::chrono::time_point<std::chrono::system_clock> midnightAtExchange(time_t today);

    /***
     * Convert an hour (in struct tm format) into a time_t, given the date of today
     * @param today the date
     * @param hour the hour
     * @return the time_t corresponding to the inputs
    */
    time_t calculateFromTm(time_t today, tm hour);

    private:
    std::string timeZone;
    std::string liquidHours; // when the market liquidity is good (regular trading hours)
    std::string tradingHours; // trading hours, including pre and post-market
    hours exchangeHours; // the exchange hours for the current day
};

