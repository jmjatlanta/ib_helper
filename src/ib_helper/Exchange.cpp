#include "Exchange.hpp"
#include "date/tz.h"

#include <iostream>

void parseYYYYMM(const std::string& in, tm& time)
{
    time.tm_year = strtol(in.substr(0,4).c_str(), nullptr, 10);
    time.tm_mon = strtol(in.substr(4,6).c_str(), nullptr, 10);
}

void parseHHMM(const std::string& in, tm& time)
{
    time.tm_hour = strtol(in.substr(0,2).c_str(), nullptr, 10);
    time.tm_min = strtol(in.substr(2, 4).c_str(), nullptr, 10);
}
/****
 * Parse hour string in the format YYYYMMDD:HHMM-HHMM,HHMM-HHMM;...
 *                              or YYYYMMDD:HHMM-HHMM;...
 * @param in the string to parse
 * @return the opening and closing hour
 */
hours parseHourFormatA(const std::string& in)
{
    hours retVal;
    int commaPos = in.find(",");
    std::string toParse = in.substr(0, commaPos);
    int colonPos = toParse.find(":");
    std::string yyyymm = toParse.substr(0, colonPos);
    parseYYYYMM(yyyymm, retVal.open);
    parseYYYYMM(yyyymm, retVal.close);
    int dashPos = toParse.find("-");
    std::string hhmm = toParse.substr(colonPos+1, dashPos);
    parseHHMM(hhmm, retVal.open);
    hhmm = toParse.substr(dashPos+1);
    parseHHMM(hhmm, retVal.close);
    return retVal;
}

/****
 * Parse hour string in the format YYYYMMDD:HHMM-YYYYMMDD:HHMM;...
 * @param in the string to parse
 * @return the opening and closing hour
 */
hours parseHourFormatB(const std::string& in)
{
    hours retVal;
    int dashPos = in.find("-");
    std::string toParse = in.substr(0, dashPos);
    int colonPos = toParse.find(":");
    std::string yyyymm = toParse.substr(0, colonPos);
    parseYYYYMM(yyyymm, retVal.open);
    std::string hhmm = toParse.substr(colonPos+1, colonPos+3);
    parseHHMM(hhmm, retVal.open);
    int semiColon = in.find(";");
    toParse = in.substr(dashPos+1, semiColon);
    colonPos = toParse.find(":");
    yyyymm = toParse.substr(0, colonPos);
    parseYYYYMM(yyyymm, retVal.close);
    hhmm = toParse.substr(colonPos + 1, colonPos + 3);
    parseHHMM(hhmm, retVal.close);
    return retVal;
}

/***
 * Parse hour string
 * NOTE: Format A is YYYYMMDD:HHMM-HHMM,HHMM-HHMM;...
 *       Format B is YYYYMMDD:HHMM-YYYYMMDD:HHMM;....
 * So the difference is in how the dates/times are separated. Format B is more
 * flexible, as it can go across days.
 * @param in the string to parse
 * @return the hours they are open
 */
hours parseHourString(const std::string& in)
{
    // format A has a dash and then 4 digits and then a non-digit
    int dashPos = in.find("-");
    int semiColonPos = in.find(";");
    int commaPos = in.find(",");
    // are we in format A or format B?
    if (commaPos != std::string::npos
            || semiColonPos - dashPos == 5)
    {
        return parseHourFormatA(in);
    }
    return parseHourFormatB(in);
}

Exchange::Exchange(const ContractDetails& contractDetails)
{
    timeZone = contractDetails.timeZoneId;
    liquidHours = contractDetails.liquidHours;
    exchangeHours = parseHourString(liquidHours);
    // LiquidHours can be in 2 formats
    // 20090507:0700-1830,1830-2330;20090508:CLOSED
    // 20180323:0400-20180323:2000;20180326:0400-20180326:2000
}

std::chrono::time_point<std::chrono::system_clock> Exchange::midnightAtExchange(time_t today)
{
    auto exchangeTimeZone = date::make_zoned(date::locate_zone(timeZone), std::chrono::system_clock::from_time_t(today));
    // now get the offset
    auto offset = exchangeTimeZone.get_info().offset;
    // now get midnight UTC and add offset
    auto midnight = date::floor<date::days>(std::chrono::system_clock::from_time_t(today));
    auto result = midnight - offset;
    return result;
}

/***
 * When does the market open for trading today?
 * @param today the time (just to get the date)
 * @return the time premarket trading starts (often Midnight)
 */
time_t Exchange::premarketStart(time_t today)
{
    auto midnight = midnightAtExchange(today);
    // now compute time_t of today
    return std::chrono::system_clock::to_time_t(midnight);
}

time_t Exchange::marketOpen(time_t today)
{
    auto clock = midnightAtExchange(today);
    // now add time to make it 9:30

    clock += std::chrono::hours(exchangeHours.open.tm_hour);
    clock += std::chrono::minutes(exchangeHours.open.tm_min);
    return std::chrono::system_clock::to_time_t(clock);
}

time_t Exchange::marketClose(time_t today)
{
    auto clock = midnightAtExchange(today);
    // now add time to make it 4pm 
    clock += std::chrono::hours(exchangeHours.close.tm_hour);
    clock += std::chrono::minutes(exchangeHours.close.tm_min);
    return std::chrono::system_clock::to_time_t(clock);
}

