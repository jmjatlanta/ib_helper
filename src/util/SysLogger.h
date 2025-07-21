#pragma once
#include <string>
#include <sstream>
#include <chrono>
#include <iostream>
#include <iomanip>
#include <mutex>
#include <ctime>

#undef ERROR

namespace util {

class SysLogger
{
    public:
    enum class LogLevel {
            DEBUG,
            INFO,
            WARN,
            ERROR
    };

    protected:
    static constexpr bool includeMillis = true;
    static constexpr bool threadSafe = false;
    std::ostream* stream = &std::cerr;
    LogLevel logLevel = LogLevel::DEBUG;
    std::mutex logMutex;

    public:
    static SysLogger* getInstance() 
    {
        static SysLogger s;
        return &s; 
    }
    void set_stream(std::ostream& in) { stream = &in; }
    void set_log_level(LogLevel in) { logLevel = in; }
    void debug(const std::string& msg) { debug("unknown", msg); }
    void debug(const std::string& category, const std::string& msg) { log(LogLevel::DEBUG, category, msg); }
    void error(const std::string& msg) { error("unknown", msg); }
    void error(const std::string& category, const std::string& msg) { log(LogLevel::ERROR, category, msg); }
    void info(const std::string& msg) { info("unknown", msg); }
    void info(const std::string& category, const std::string& msg) { log(LogLevel::INFO, category, msg); }
    void warn(const std::string& msg) { warn("unknown", msg); }
    void warn(const std::string& category, const std::string& msg) { log(LogLevel::WARN, category, msg); }
    void log(LogLevel level, const std::string& category, const std::string& msg);

    protected:
    SysLogger() {}

    private:
    std::string current_time_formatted()
    {
        std::chrono::system_clock::time_point t = std::chrono::system_clock::now();
        time_t now = std::chrono::system_clock::to_time_t(t);
        tm* lt = localtime(&now);

        std::stringstream ss;
        ss << (1900 + lt->tm_year) << '-'
            << std::setfill('0') << std::setw(2) << (lt->tm_mon + 1) << '-'
            << std::setfill('0') << std::setw(2) << lt->tm_mday << ' '
            << std::setfill('0') << std::setw(2) << lt->tm_hour << ':'
            << std::setfill('0') << std::setw(2) << lt->tm_min << ':'
            << std::setfill('0') << std::setw(2) << lt->tm_sec;
        if (includeMillis)
        {
            const std::chrono::duration<double> tse = t.time_since_epoch();
            std::chrono::seconds::rep milliseconds = 
                std::chrono::duration_cast<std::chrono::milliseconds>(tse).count() % 1000;
            ss << '.' << std::setfill('0') << std::setw(3) << milliseconds;
        }
        return ss.str();
    }
};

} // namespace util

std::string to_string(util::SysLogger::LogLevel in);

util::SysLogger::LogLevel to_log_level(const std::string& in);

