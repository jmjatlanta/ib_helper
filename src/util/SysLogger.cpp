#include "SysLogger.h"

namespace util
{

void SysLogger::log(LogLevel level, const std::string& category, const std::string& msg)
{
    if (static_cast<uint32_t>(level) >= static_cast<uint32_t>(logLevel))
    {
        if (threadSafe)
        {
            std::lock_guard<std::mutex> lock(logMutex);
            *stream << current_time_formatted() << " "
                    << ::to_string(level) << " [" << category << "] " << msg << "\n";
        }
        else
        {
            *stream << current_time_formatted() << " "
                    << to_string(level) << " [" << category << "] " << msg << "\n";
        }
    }
}

} // namespace util

std::string to_string(util::SysLogger::LogLevel in) {
    switch (in) {
        case util::SysLogger::LogLevel::DEBUG:
            return "DEBUG";
        case util::SysLogger::LogLevel::INFO:
            return "INFO";
        case util::SysLogger::LogLevel::WARN:
            return "WARN";
        case util::SysLogger::LogLevel::ERROR:
            return "ERROR";
        default:
            return "";
    }
}

util::SysLogger::LogLevel to_log_level(const std::string& in)
{
    if (in == "DEBUG")
        return util::SysLogger::LogLevel::DEBUG;
    if (in == "INFO")
        return util::SysLogger::LogLevel::INFO;
    if (in == "WARN")
        return util::SysLogger::LogLevel::WARN;
    if (in == "ERROR")
        return util::SysLogger::LogLevel::ERROR;
    return util::SysLogger::LogLevel::DEBUG;
}
