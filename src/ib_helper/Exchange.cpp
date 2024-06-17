#include "Exchange.hpp"
#include "date/tz.h"
#include <iomanip>
#include <iostream>

void parseYYYYMM(const std::string& in, tm& time)
{
    time.tm_year = strtol(in.substr(0,4).c_str(), nullptr, 10);
    time.tm_mon = strtol(in.substr(4,6).c_str(), nullptr, 10);
}

/****
 * @param in the time in format HHMM, hours must be 2 digits
 * @param time a tm struct to fill
 */
void parseHHMM(const std::string& in, tm& time)
{
    if (in.size() < 4)
        throw std::invalid_argument("Format should be HHMM. Input: " + in); // NOTE: extra characters okay
    if (in.find(":") != std::string::npos)
        throw std::invalid_argument("Format should be HHMM, without colon. Input: " + in);

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
    parseYYYYMM(yyyymm, retVal.start);
    parseYYYYMM(yyyymm, retVal.stop);
    int dashPos = toParse.find("-");
    std::string hhmm = toParse.substr(colonPos+1, dashPos);
    parseHHMM(hhmm, retVal.open);
    parseHHMM(hhmm, retVal.start);
    hhmm = toParse.substr(dashPos+1);
    parseHHMM(hhmm, retVal.close);
    parseHHMM(hhmm, retVal.stop);
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
    parseYYYYMM(yyyymm, retVal.start);
    std::string hhmm = toParse.substr(colonPos+1, colonPos+3);
    parseHHMM(hhmm, retVal.open);
    parseHHMM(hhmm, retVal.start);
    int semiColon = in.find(";");
    toParse = in.substr(dashPos+1, semiColon);
    colonPos = toParse.find(":");
    yyyymm = toParse.substr(0, colonPos);
    parseYYYYMM(yyyymm, retVal.close);
    parseYYYYMM(yyyymm, retVal.stop);
    hhmm = toParse.substr(colonPos + 1, colonPos + 3);
    parseHHMM(hhmm, retVal.close);
    parseHHMM(hhmm, retVal.stop);
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
    : logger(Logger::getInstance())
{
    timeZone = contractDetails.timeZoneId;
    liquidHours = contractDetails.liquidHours;
    exchangeHours = parseHourString(liquidHours);
    // LiquidHours can be in 2 formats
    // 20090507:0700-1830,1830-2330;20090508:CLOSED
    // 20180323:0400-20180323:2000;20180326:0400-20180326:2000
    tradingHours = contractDetails.tradingHours;
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

std::string removeColon(const std::string& in)
{
    // split into digits
    auto pos = in.find(":");
    if (pos == std::string::npos) // no colon
        return in;
    uint8_t hr = strtol(in.substr(0, pos).c_str(), nullptr, 10);
    uint8_t min = strtol(in.substr(pos + 1).c_str(), nullptr, 10);
    std::stringstream ss;
    ss << std::fixed << std::setw(2) << std::setfill('0') << std::right << (int)hr
        << std::fixed << std::setw(2) << std::setfill('0') << std::right << (int)min;
    return ss.str();
}

void Exchange::setStartTime(const std::string& in)
{
    // get rid of the colon if there is one
    std::string cleaned = removeColon(in);
    parseHHMM(cleaned, exchangeHours.start);
}

void Exchange::setStopTime(const std::string& in)
{
    // get rid of the colon if there is one
    std::string cleaned = removeColon(in);
    parseHHMM(cleaned, exchangeHours.stop);
}

bool Exchange::isWithinRange(time_t in)
{
    bool retval = in > marketStart(in) && in << marketStop(in);
    if (!retval)
    {
        logger->debug("Exchange", "Time out of range. Time: " + std::to_string(in)
                + " start: " + std::to_string(marketStart(in))
                + " stop: " + std::to_string(marketStop(in)));

    }
    return in > marketStart(in) && in < marketStop(in);
}

time_t Exchange::calculateFromTm(time_t today, tm hour)
{
    auto clock = midnightAtExchange(today);
    clock += std::chrono::hours(hour.tm_hour);
    clock += std::chrono::minutes(hour.tm_min);
    return std::chrono::system_clock::to_time_t(clock);
}

/***
 * When does the market open for trading today?
 * @param today the time (just to get the date)
 * @return the time premarket trading starts (often Midnight)
 */
time_t Exchange::premarketStart(time_t today)
{
    tm midnight = {0};
    return calculateFromTm(today, midnight);
}

time_t Exchange::marketOpen(time_t today)
{
    return calculateFromTm(today, exchangeHours.open);
}

time_t Exchange::marketClose(time_t today)
{
    return calculateFromTm(today, exchangeHours.close);
}

time_t Exchange::marketStart(time_t today)
{
    return calculateFromTm(today, exchangeHours.start);
}

time_t Exchange::marketStop(time_t today)
{
    return calculateFromTm(today, exchangeHours.stop);
}

