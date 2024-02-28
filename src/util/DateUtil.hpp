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

#ifdef MOCK_TIME
void set_current_time(time_t in);
#endif
time_t current_time();

/***
 * Split "2024-01-01 18:59" into 18 hours, 59 minutes
 * @param in the string
 * @return the hour and minute
*/
std::pair<uint16_t, uint16_t> split_time(const std::string& in);

/***
 * Convert a time (such as "18:59" to a time_t based on the current time
 * @param in the time to convert, as a a string
 * @param now the current time
 * @return today at the passed in time
*/
std::time_t to_time_t(const std::string& in, std::time_t now);