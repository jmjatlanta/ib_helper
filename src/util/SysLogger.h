#pragma once
#include <string>
#include <sstream>
#include <chrono>
#include <iostream>
#include <iomanip>
#ifdef WIN32
#define localtime_r(_Time, _Tm) localtime_s(_Tm, _Time)
#endif

namespace util {

constexpr bool includeMillis = true;

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
    std::ostream* stream = &std::cerr;
    LogLevel logLevel = LogLevel::WARN;

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
    void log(LogLevel level, const std::string& category, const std::string& msg)
    {
        if (static_cast<uint32_t>(level) >= static_cast<uint32_t>(logLevel))
        {
            auto in_time_t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
            *stream << current_time_formatted() << " "
                    << to_string(level) << " [" << category << "] " << msg << "\n";
        }
    }
    static std::string to_string(LogLevel in) {
        switch (in) {
            case LogLevel::DEBUG:
                return "DEBUG";
            case LogLevel::INFO:
                return "INFO";
            case LogLevel::WARN:
                return "WARN";
            case LogLevel::ERROR:
                return "ERROR";
            default:
                return "";
        }
    }
    protected:
    SysLogger() {}

    private:
    std::string current_time_formatted()
    {
        tm localTime;
        std::chrono::system_clock::time_point t = std::chrono::system_clock::now();
        time_t now = std::chrono::system_clock::to_time_t(t);
        localtime_r(&now, &localTime);

        std::stringstream ss;
        ss << (1900 + localTime.tm_year) << '-'
            << std::setfill('0') << std::setw(2) << (localTime.tm_mon + 1) << '-'
            << std::setfill('0') << std::setw(2) << localTime.tm_mday << ' '
            << std::setfill('0') << std::setw(2) << localTime.tm_hour << ':'
            << std::setfill('0') << std::setw(2) << localTime.tm_min << ':'
            << std::setfill('0') << std::setw(2) << localTime.tm_sec;
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
