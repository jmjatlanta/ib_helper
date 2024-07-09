#pragma once
#include <cstdint>
#include <string>

/****
 * Get only valid IB bar sizes
*/
namespace ib_helper
{

enum class BarSize
{
    UNKNOWN = 0,
    ONE_SECOND,
    FIVE_SECONDS,
    TEN_SECONDS,
    FIFTEEN_SECONDS,
    THIRTY_SECONDS,
    ONE_MINUTE,
    TWO_MINUTES,
    THREE_MINUTES,
    FIVE_MINUTES,
    TEN_MINUTES,
    FIFTEEN_MINUTES,
    TWENTY_MINUTES,
    THIRTY_MINUTES,
    ONE_HOUR,
    TWO_HOURS,
    THREE_HOURS,
    FOUR_HOURS,
    EIGHT_HOURS,
    ONE_DAY,
    ONE_WEEK,
    ONE_MONTH,
    COUNT_
};

enum class DurationUnit
{
    UNKNOWN = 0,
    SECOND,
    DAY,
    WEEK,
    MONTH,
    YEAR
};

} // namespace ib_helper

/***
 * @brief build string useful for UI components
 *
 * @returns string like "1 minute" or "4 hours"
 */
std::string to_string(ib_helper::BarSize in);

/***
 * @brief build string of bar length for use with IB
 *
 * @returns string like "1 min" or "4 hours"
 */
std::string to_ib_string(ib_helper::BarSize in);

/***
 * @brief build a string that corresponds to the duration unit
 * @return a string such as "Seconds" or "Week"
 */
std::string to_string(ib_helper::DurationUnit in);

/***
 * @return a string for use with IB such as "S" for seconds or "M" for month
 */
std::string to_ib_string(ib_helper::DurationUnit in);

/***
 * @return a correctly formatted "duration string" for getting historical bars
 */
std::string to_ib_string(uint16_t qty, ib_helper::DurationUnit duration);

