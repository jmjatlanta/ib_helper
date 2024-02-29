#pragma once
#include <string>

/****
 * Get only valid IB bar sizes
*/
namespace ib_helper
{

enum class BarSize
{
    ONE_SECOND = 0,
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

} // namespace ib_helper

std::string to_string(ib_helper::BarSize in);

std::string to_ib_string(ib_helper::BarSize in);