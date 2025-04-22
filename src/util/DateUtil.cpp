#include "DateUtil.hpp"
#include "bar.h"
#include <iomanip>
#include <sstream>
#include <utility>
#include <cstring>
#include <chrono>

#ifdef _WIN32
#define timegm _mkgmtime
#endif
#ifdef HH_DATELIB
#include "date/tz.h"
#endif

/***
 * @brief convert a string "time" into a time point
 * @param date the date portion
 * @param hhmm the time in HH:MM format
 */
time_pnt to_time_point(const time_pnt& date, const std::string& hhmm)
{
    time_t midnight = to_midnight_ny(std::chrono::system_clock::to_time_t(date));
    auto pr = split_time(hhmm);
    // add in the hours and minutes
    midnight += (pr.first * 60 * 60); // add hours
    midnight += (pr.second * 60); // add minutes
    return std::chrono::system_clock::from_time_t(midnight);
}

/***
 * @brief convert a string into a time point
 * @param in the string in the format YYYYMMDD HH:MM:SS time/zone OR in time_t format
 * @return the matching time_point<system_clock>
 */
time_pnt to_time_point(const std::string& in)
{
    // which format are we in? If we have only digits, we're in time_t format
    auto pos = in.find_last_of(' ');
    if (pos == std::string::npos)
    {
        // we're in time_t format
        return std::chrono::system_clock::from_time_t( std::strtoul( in.c_str(), nullptr, 10) );
    }
    // we must get the time zone information
    std::string tzString = in.substr( pos + 1);
    std::string tz_name;
    std::istringstream ss(in);
#ifdef HH_DATELIB
    date::local_time<std::chrono::milliseconds> resultTime;
    ss >> date::parse("%Y%m%d %H:%M:%S %Z", resultTime);
    // now we need to convert it to another
    auto ny_time = date::zoned_time<std::chrono::milliseconds>(tzString, resultTime); // converted GMT to NY, but not what we want
#else
    std::chrono::local_time<std::chrono::milliseconds> resultTime;
    ss >> std::chrono::parse("%Y%m%d %H:%M:%S %Z", resultTime, tz_name);
    // now we need to convert it to another
    auto ny_time = std::chrono::zoned_time<std::chrono::milliseconds>(tzString, resultTime); // converted GMT to NY, but not what we want
#endif
    return ny_time.get_sys_time();
}

std::string to_string(const time_pnt& in)
{
#ifdef HH_DATELIB
    return date::format("%D %T %Z", floor<std::chrono::milliseconds>(in));
#else
    return std::format("{:%Y%m%d%H%M}", in);
#endif
}

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

DayOfWeek to_day_of_week(time_t in)
{
    auto tm = *gmtime(&in);
    return static_cast<DayOfWeek>(tm.tm_wday);
}

bool is_monday(time_t in) { return to_day_of_week(in) == DayOfWeek::MONDAY; }
bool is_friday(time_t in) { return to_day_of_week(in) == DayOfWeek::FRIDAY; }

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
    // add 9 1/2 hours
    return to_midnight_ny(in) + 34200;
}

time_t to_4pm_ny(time_t in)
{
    // add 16 hours
    return to_midnight_ny(in) + 57600;
}

time_t to_minute_floor(time_t in)
{
    return (in / 60) * 60;
}

int32_t diff_with_ny(std::time_t now)
{
    auto utc = std::chrono::system_clock::from_time_t(now);
#ifdef HH_DATELIB
    date::zoned_time ny_time = date::make_zoned("America/New_York", utc);
#else
    std::chrono::zoned_time ny_time{"America/New_York", utc};
#endif
    auto cnt = ny_time.get_info().offset;
    return cnt.count();
}

std::time_t to_ny_time(std::time_t now)
{
    return now - diff_with_ny(now);
}

uint32_t secs_since_midnight_utc(time_t in)
{
    auto tm = *gmtime(&in);
    return (tm.tm_hour * 3600) + (tm.tm_min * 60) + tm.tm_sec;
}

time_t to_midnight_ny(time_t in)
{
    int32_t diff = diff_with_ny(in);
    // NOTE: If the diff_with_ny is greater than the secs since midnight UTC, the day of year rolled over
    // in UTC, so we'll need to back that out.
    // convert to midnight UTC
    auto tm = *gmtime(&in);
    //tm.tm_gmtoff = 0;
    //tm.tm_zone = "Etc/UTC";
    tm.tm_hour = 0;
    tm.tm_min = 0;
    tm.tm_sec = 0;
    if (secs_since_midnight_utc(in) < (diff * -1))
        tm.tm_mday--;
    time_t temp =  timegm(&tm);
    return temp - diff_with_ny(in);
}

time_t mock_time = 0;
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
    retval.first = 0;
    retval.second = 0;
    // find the colon
    auto pos = in.find(":");
    if (pos != std::string::npos && pos >= 1)
    {
        retval.first = strtol(in.substr(0, pos).c_str(), nullptr, 10);
        retval.second = strtol(in.substr(pos+1).c_str(), nullptr, 10);
    }
    else
    {
        // no colon
        if (!in.empty())
            retval.first = strtol(in.c_str(), nullptr, 10);
    }
    return retval;
}

template<class T>
std::string leadZeros(T num, uint16_t numPlaces)
{
    std::string result = std::to_string(num);
    while (numPlaces > result.size())
        result = "0" + result;
    return result;
}

/****
 * @brief clean up time
 * @note valid values are [0-23]:[0-59]
 * @param in the time in the format HH:MM
 * @param pm TRUE if we believe this is PM
 * @return the time in 24 hour format. if in.empty(), returns empty string
 */
std::string cleanTime(const std::string& in, bool pm)
{
    if (in.empty())
        return in;
    auto timePair = ::split_time(in);
    if (timePair.first == 12)
    {
        // AM/PM has to rely on passed in parameter
        if (!pm)
            timePair.first = 0;
    }
    else
    {
        if (timePair.first < 12)
        {
            if (pm)
                timePair.first += 12;
            if (timePair.first == 24)
                timePair.first = 0; 
        }
    }
    return std::to_string(timePair.first) + ":" + leadZeros(timePair.second, 2);
}

/****
 * @brief convert time string in format HH:MM to 12 hour format
 * @param in the input
 * @returns the time and a boolean that is TRUE if the time is PM
 */
std::pair<std::string, bool> to_12hr_format(const std::string& in)
{
    std::string cleaned = in;
    auto pair = split_time(cleaned);
    bool pm = (pair.first >= 12);
    if (pm && pair.first > 12)
        pair.first -= 12;
    if (pair.first == 0)
        pair.first = 12;
    cleaned = std::to_string(pair.first) + ":" + leadZeros(pair.second, 2);
    return std::make_pair(cleaned, pm );
}

/***
 * @brief given a string in the format HH:MM convert to time_t
 * @param in the string in the format HH:MM, assuming NY timezone
 * @param now the current system time
 * @return a time_t that represents the date of (now) with the time of (in)
 */
std::time_t to_time_t(const std::string& in, std::time_t now)
{
    std::pair<uint16_t, uint16_t> hhmm = split_time(in);
    // adjust for GMT
    int32_t diff = diff_with_ny(now);
    hhmm.first -= diff / 3600;
    hhmm.second -= diff % 3600;
    auto tm = *gmtime(&now);
    tm.tm_hour = hhmm.first;
    tm.tm_min = hhmm.second;
    tm.tm_sec = 0;
    return timegm(&tm);
}

/***
 * @param bar the bar
 * @return the time_t of the bar time
*/
std::time_t to_time_t(const Bar& bar)
{
    // compute time
    // if > 30000000, the time is time_t
    time_t l = strtol(bar.time.c_str(), nullptr, 10);
    if (bar.time.size() == 8)
    {
        // convert from YYYYMMDD
        std::string newTime = bar.time.substr(0,4) + "-" + bar.time.substr(4, 2) + "-" + bar.time.substr(6, 2) + " 09:30:00";
        std::time_t temp = ::time(nullptr);
        tm t = *gmtime(&temp);
        std::istringstream ss(newTime);
        ss >> std::get_time(&t, "%Y-%m-%d %H:%M:%S");
        time_t tempTime = mktime(&t);
        l = to_midnight_ny(tempTime);
    }
    return l;
}

time_pnt to_time_point(const Bar& bar)
{
    return std::chrono::system_clock::from_time_t(to_time_t(bar));
}
