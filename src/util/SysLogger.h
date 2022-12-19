#include <string>
#include <iostream>

namespace util {

class SysLogger
{
    enum LogLevel {
            DEBUG,
            INFO,
            WARN,
            ERROR
    };

    public:
    static SysLogger* getInstance() 
    {
        static SysLogger s;
        return &s; 
    }
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
        std::cerr << to_string(level) << " [" << category << "] " << msg << "\n";
    }
    static std::string to_string(LogLevel in) {
        switch (in) {
            case DEBUG:
                return "DEBUG";
            case INFO:
                return "INFO";
            case WARN:
                return "WARN";
            case ERROR:
                return "ERROR";
            default:
                return "";
        }
    }
    protected:
    SysLogger() {}
};

} // namespace util
