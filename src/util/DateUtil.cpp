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

bool is_monday(time_t in)
{
    auto tm = *gmtime(&in);
    return tm.tm_wday == 1;
}

bool is_friday(time_t in)
{
    auto tm = *gmtime(&in);
    return tm.tm_wday == 5;
}

time_t to_previous_friday(time_t in)
{
    auto tm = *gmtime(&in);
    if (tm.tm_wday < 6) // sun through fri
        tm.tm_mday -= (2 + tm.tm_wday);
    tm.tm_mday -= 1; // sat
    return mktime(&tm);
}
time_t to_next_friday(time_t in)
{
    auto tm = *gmtime(&in);
    if (tm.tm_wday < 5) // sun through thurs
        tm.tm_mday += (5 - tm.tm_wday);
    tm.tm_mday += 7 - (tm.tm_wday-5); // fri and sat
    return mktime(&tm);
}

time_t to_930am_ny(time_t in)
{
    auto tm = *gmtime(&in);
    //tm.tm_gmtoff = 0;
    //tm.tm_zone = "Etc/UTC";
    tm.tm_hour = 14;
    tm.tm_min = 30;
    tm.tm_sec = 0;
#ifdef _WIN32
    return _mkgmtime(&tm);
#else
    return timegm(&tm);
#endif
}

time_t to_4pm_ny(time_t in)
{
    auto tm = *gmtime(&in);
    //tm.tm_gmtoff = 0;
    //tm.tm_zone = "Etc/UTC";
    tm.tm_hour = 21;
    tm.tm_min = 0;
    tm.tm_sec = 0;
#ifdef _WIN32
    return _mkgmtime(&tm);
#else
    return timegm(&tm);
#endif
}

time_t to_midnight_ny(time_t in)
{
    auto tm = *gmtime(&in);
    //tm.tm_gmtoff = 0;
    //tm.tm_zone = "Etc/UTC";
    tm.tm_hour = 5;
    tm.tm_min = 0;
    tm.tm_sec = 0;
#ifdef _WIN32
    return _mkgmtime(&tm);
#else
    return timegm(&tm);
#endif
}

time_t mock_time;
void set_current_time(time_t in) { mock_time = in; }

time_t current_time()
{
    if (mock_time > 0)
        return mock_time;
    return ::time(nullptr);
}

std::pair<uint16_t, uint16_t> split_time(const std::string& in)
{
    std::pair<uint16_t, uint16_t> retval;
    // find the colon
    auto pos = in.find(":");
    if (pos != std::string::npos && pos >= 1)
    {
        retval.first = strtol(in.substr(0, pos).c_str(), nullptr, 10);
        retval.second = strtol(in.substr(pos+1).c_str(), nullptr, 10);
    }
    return retval;
}

std::time_t to_time_t(const std::string& in, std::time_t now)
{
    std::pair<uint16_t, uint16_t> hhmm = split_time(in);
    auto tm = *gmtime(&now);
    tm.tm_hour = hhmm.first;
    tm.tm_min = hhmm.second;
    tm.tm_sec = 0;
    return mktime(&tm);
}
