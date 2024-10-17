#pragma once
#include <string>
#include <ctime>
#include <chrono>

struct Bar; // an IB bar
typedef std::chrono::time_point<std::chrono::system_clock> time_pnt;

/***
 * @brief convert a string into a time point
 * @param in the string in the format YYYYMMDD HH:MM:SS time/zone or UNIX epoch as string
 * @return the matching time_point<system_clock>
 */
time_pnt to_time_point(const std::string& in);

std::string to_string(const time_pnt& in);

/***
 * convert today into YYYYMMDD
 * @return string of YYYYMMDD
 */
std::string getDate();

/***
 * Convert today into time_t, but adjusted to a certain hour/minute
 * @param day today
 * @param hour the hour to adjust to (GMT)
 * @param minute the minute to adjust to (GMT)
 * @returns the time_t of the epoch, adjusted to the hour/minute
 */
time_t timeToEpoch(time_t day, uint32_t hour, uint32_t minute);

/***
 * @param in the time
 * @returns true if in is a monday GMT
 */
bool is_monday(time_t in);
/***
 * @param in the time
 * @returns true if in is a friday GMT
 */
bool is_friday(time_t in);

/***
 * @param in the time
 * @return the prior friday
 */
time_t to_previous_friday(time_t in);
/***
 * @param in the time
 * @return the next friday
 */
time_t to_next_friday(time_t in);

/***
 * @param in the time
 * @return 9:30am in NY on the same day
 */
time_t to_930am_ny(time_t in);
/***
 * @param in the time
 * @return 4pm in NY on the same day
 */
time_t to_4pm_ny(time_t in);
/***
 * @param in the time
 * @return midnight in NY on the same day
 */
time_t to_midnight_ny(time_t in);

/***
 * @brief remove seconds
 * @param in the time
 * @return the time rounded down to the minute
 */
time_t to_minute_floor(time_t in);

/***
 * @brief given a string in the format HH:MM convert to time_t
 * @param in the string, with the hour and minute in the NY time zone
 * @param now the current system time
 * @return a time_t that represents the date of (now) with the time of (in)
 */
std::time_t to_time_t(const std::string& in, std::time_t now);

/**
 * @brief convert a bar time to time_t
 * @NOTE: bars can have their time in time_t (short duration bars) or dates (daily bars)
 * 
 * @param bar the bar
 * @return the time
 */
std::time_t to_time_t(const Bar& bar);

/**
 * @brief convert a bar time to a duration
 * @NOTE: bars can have their time in time_t (short duration bars) or dates (daily bars)
 * 
 * @param bar the bar
 * @return the time
 */
time_pnt to_time_point(const Bar& bar);

/**
 * @brief split a string "HH:MM" into their 2 parts
 * 
 * @param in the string
 * @return the two values hour and minute
 */
std::pair<uint16_t, uint16_t> split_time(const std::string& in);

/****
 * @brief clean up time
 * @note valid values are [0-23]:[0-59]
 * @param in the time in the format HH:MM
 * @param pm TRUE if we believe this is PM
 * @return the time in 24 hour format. if in.empty(), returns empty string
 */
std::string cleanTime(const std::string& in, bool pm);

/****
 * @brief convert time string in format HH:MM to 12 hour format
 * @param in the input
 * @returns the time and a boolean that is TRUE if the time is PM
 */
std::pair<std::string, bool> to_12hr_format(const std::string& in);

#ifdef MOCK_TIME
void set_current_time(time_t in);
#endif

time_t current_time();

/***
 * @brief how UTC compares to NY
 * @param now the time you want to compare
 * @return the seconds difference between NY and UTC on the date/time given
*/
int32_t diff_with_ny(std::time_t now);
