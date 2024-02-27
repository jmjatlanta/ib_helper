#include "DateUtil.hpp"
#include <iomanip>
#include <sstream>
#include <cstring>

/***
 * convert today into YYYYMMDD
 * @return string of YYYYMMDD
 */
std::string getDate() 
{
    std::time_t currTime = std::time(0);
    std::tm* now = std::localtime(&currTime);
    std::stringstream ss;
    ss << (now->tm_year + 1900) 
        << std::setw(2) << std::setfill('0') << now->tm_mon + 1 
        << std::setw(2) << std::setfill('0') << now->tm_mday;
    return ss.str();
}

/***
 * Convert today into time_t, but adjusted to a certain hour/minute
 * @param day today
 * @param hour the hour to adjust to
 * @param minute the minute to adjust to
 * @returns the time_t of the epoch, adjusted to the hour/minute
 */
time_t timeToEpoch(time_t day, uint32_t hour, uint32_t minute)
{
    // get "today"
    std::tm today;
    std::memcpy(&today, localtime(&day), sizeof(std::tm));
    // adjust hour and minute
    today.tm_hour = hour;
    today.tm_min = minute;
    today.tm_sec = 0;
    // now convert back to epoch
    return mktime(&today);
}

time_t to_next_friday(time_t in)
{
    auto tm = *gmtime(&in);
    if (tm.tm_wday < 5)
        tm.tm_mday += (5 - tm.tm_wday);
    if (tm.tm_wday == 6)
        tm.tm_mday += 4;
    return mktime(&tm);
}

time_t to_4pm_ny(time_t in)
{
    auto tm = *gmtime(&in);
    tm.tm_gmtoff = 0;
    tm.tm_zone = "Etc/UTC";
    tm.tm_hour = 21;
    tm.tm_min = 0;
    tm.tm_sec = 0;
    return timegm(&tm);
}


