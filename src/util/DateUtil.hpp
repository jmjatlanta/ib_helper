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

#ifdef MOCK_TIME

time_t mock_time;
void set_current_time(time_t in) { mock_time = in; }
#endif

time_t current_time();
