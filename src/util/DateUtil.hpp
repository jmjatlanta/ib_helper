#pragma once
#include <string>
#include <ctime>

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

time_t to_next_friday(time_t in);

time_t to_4pm_ny(time_t in);

/***
 * @param in a time as a string in the format "HH:MM"
 * @param now a date/time to be adjusted
 * @return now, adjusted by in
*/
std::time_t to_time_t(const std::string& in, std::time_t now);
/**
 * @brief split a string "HH:MM" into their 2 parts
 * 
 * @param in  the string
 * @return  the two values
 */
std::pair<uint16_t, uint16_t> split_time(const std::string& in);

#ifdef MOCK_TIME
void set_current_time(time_t in);
#endif

time_t current_time();
