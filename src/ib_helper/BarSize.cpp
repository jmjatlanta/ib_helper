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
    }
    return "UNKNOWN";
};
