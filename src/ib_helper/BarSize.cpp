#include "BarSize.hpp"

using ib_helper::BarSize;

std::string to_string(ib_helper::BarSize in)
{
    switch(in)
    {
        case BarSize::ONE_SECOND:
            return "1 second";
        case BarSize::FIVE_SECONDS:
            return "5 seconds";
        case BarSize::TEN_SECONDS:
            return "10 seconds";
        case BarSize::FIFTEEN_SECONDS:
            return "15 seconds";
        case BarSize::THIRTY_SECONDS:
            return "30 seconds";
        case BarSize::ONE_MINUTE:
            return "1 minute";
        case BarSize::TWO_MINUTES:
            return "2 minutes";
        case BarSize::THREE_MINUTES:
            return "3 minutes";
        case BarSize::FIVE_MINUTES:
            return "5 minutes";
        case BarSize::TEN_MINUTES:
            return "10 minutes";
        case BarSize::FIFTEEN_MINUTES:
            return "15 minutes";
        case BarSize::TWENTY_MINUTES:
            return "20 minutes";
        case BarSize::THIRTY_MINUTES:
            return "30 minutes";
        case BarSize::ONE_HOUR:
            return "1 hour";
        case BarSize::TWO_HOURS:
            return "2 hours";
        case BarSize::THREE_HOURS:
            return "3 hours";
        case BarSize::FOUR_HOURS:
            return "4 hours";
        case BarSize::EIGHT_HOURS:
            return "8 hours";
        case BarSize::ONE_DAY:
            return "1 day";
        case BarSize::ONE_WEEK:
            return "1 week";
        case BarSize::ONE_MONTH:
            return "1 month";
        default:
            break;
    }
    return "UNKNOWN";
}

std::string to_ib_string(ib_helper::BarSize in)
{
    switch(in)
    {
        case BarSize::ONE_SECOND:
            return "1 secs";
        case BarSize::FIVE_SECONDS:
            return "5 secs";
        case BarSize::TEN_SECONDS:
            return "10 secs";
        case BarSize::FIFTEEN_SECONDS:
            return "15 secs";
        case BarSize::THIRTY_SECONDS:
            return "30 secs";
        case BarSize::ONE_MINUTE:
            return "1 min";
        case BarSize::TWO_MINUTES:
            return "2 mins";
        case BarSize::THREE_MINUTES:
            return "3 mins";
        case BarSize::FIVE_MINUTES:
            return "5 mins";
        case BarSize::TEN_MINUTES:
            return "10 mins";
        case BarSize::FIFTEEN_MINUTES:
            return "15 mins";
        case BarSize::TWENTY_MINUTES:
            return "20 mins";
        case BarSize::THIRTY_MINUTES:
            return "30 mins";
        case BarSize::ONE_HOUR:
            return "1 hour";
        case BarSize::TWO_HOURS:
            return "2 hours";
        case BarSize::THREE_HOURS:
            return "3 hours";
        case BarSize::FOUR_HOURS:
            return "4 hours";
        case BarSize::EIGHT_HOURS:
            return "8 hours";
        case BarSize::ONE_DAY:
            return "1 day";
        case BarSize::ONE_WEEK:
            return "1W";
        case BarSize::ONE_MONTH:
            return "1M";
        default:
            break;
    }
    return "UNKNOWN";
};

std::string to_string(ib_helper::DurationUnit in)
{
    switch(in)
    {
        case ib_helper::DurationUnit::SECOND:
            return "Seconds";
        case ib_helper::DurationUnit::DAY:
            return "Day";
        case ib_helper::DurationUnit::WEEK:
            return "Week";
        case ib_helper::DurationUnit::MONTH:
            return "Month";
        case ib_helper::DurationUnit::YEAR:
            return "Year";
        default:
            return "Unknown";
    }
    return "Unknown";
}

std::string to_ib_string(ib_helper::DurationUnit in)
{
    switch(in)
    {
        case ib_helper::DurationUnit::SECOND:
            return "S";
        case ib_helper::DurationUnit::DAY:
            return "D";
        case ib_helper::DurationUnit::WEEK:
            return "W";
        case ib_helper::DurationUnit::MONTH:
            return "M";
        case ib_helper::DurationUnit::YEAR:
            return "Y";
        default:
            return "";
    }
    return "";
}

std::string to_ib_string(uint16_t qty, ib_helper::DurationUnit duration)
{
    if (qty <= 0 || duration == ib_helper::DurationUnit::UNKNOWN)
        return "";
    return std::to_string(qty) + " " + to_ib_string(duration);
}

std::chrono::seconds to_duration(ib_helper::BarSize barSize)
{
    switch(barSize)
    {
        case ib_helper::BarSize::ONE_SECOND:
            return std::chrono::seconds(1);
        case ib_helper::BarSize::FIVE_SECONDS:
            return std::chrono::seconds(5);
        case ib_helper::BarSize::TEN_SECONDS:
            return std::chrono::seconds(10);
        case ib_helper::BarSize::FIFTEEN_SECONDS:
            return std::chrono::seconds(15);
        case ib_helper::BarSize::THIRTY_SECONDS:
            return std::chrono::seconds(30);
        case ib_helper::BarSize::ONE_MINUTE:
            return std::chrono::minutes(1);
        case ib_helper::BarSize::TWO_MINUTES:
            return std::chrono::minutes(2);
        case ib_helper::BarSize::THREE_MINUTES:
            return std::chrono::minutes(3);
        case ib_helper::BarSize::FIVE_MINUTES:
            return std::chrono::minutes(5);
        case ib_helper::BarSize::TEN_MINUTES:
            return std::chrono::minutes(10);
        case ib_helper::BarSize::FIFTEEN_MINUTES:
            return std::chrono::minutes(15);
        case ib_helper::BarSize::TWENTY_MINUTES:
            return std::chrono::minutes(20);
        case ib_helper::BarSize::THIRTY_MINUTES:
            return std::chrono::minutes(30);
        case ib_helper::BarSize::ONE_HOUR:
            return std::chrono::hours(1);
        case ib_helper::BarSize::TWO_HOURS:
            return std::chrono::hours(2);
        case ib_helper::BarSize::THREE_HOURS:
            return std::chrono::hours(3);
        case ib_helper::BarSize::FOUR_HOURS:
            return std::chrono::hours(4);
        case ib_helper::BarSize::EIGHT_HOURS:
            return std::chrono::hours(8);
        case ib_helper::BarSize::ONE_DAY:
            return std::chrono::days(1);
        case ib_helper::BarSize::ONE_WEEK:
            return std::chrono::weeks(1);
        case ib_helper::BarSize::ONE_MONTH:
            return std::chrono::months(1);
        default:
            return std::chrono::seconds(0);
    }
}
