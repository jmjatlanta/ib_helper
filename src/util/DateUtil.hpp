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
 * @param hour the hour to adjust to
 * @param minute the minute to adjust to
 * @returns the time_t of the epoch, adjusted to the hour/minute
 */
uint32_t timeToEpoch(time_t day, uint32_t hour, uint32_t minute);

