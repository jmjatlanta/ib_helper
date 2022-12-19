namespace util {

class SysLogger
{
    public:
    static SysLogger* getInstance() 
    {
        static SysLogger s;
        return &s; 
    }
    void debug(const std::string& msg) {}
    void error(const std::string& msg) {}
    void info(const std::string& msg) {}
    void warn(const std::string& msg) {}
    protected:
    SysLogger() {}
};

} // namespace util
