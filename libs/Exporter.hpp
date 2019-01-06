#pragma once

#include <fstream>
#include <vector>
#include <mutex>
#include <chrono>
#include <thread>
#include <sstream>
#include <string>

#include <cstdarg>
#include <iomanip>

#include <netinet/in.h>
#include <unistd.h> // close

namespace llt {

struct LogInfo;
class Export;

enum class LogType : uint8_t
{
    INFO=0,
    WARN,
    ERROR,
    TRACE,
};

struct Util
{
    static size_t const MAX_BUF_SIZE = 0x1000;
    static std::string SEPARATOR;

    static inline std::string to_string(std::thread::id const &aTid)
    {
        std::ostringstream ss;
        ss << aTid;
        return std::string(ss.str());
    }

    static inline std::string to_string(LogType aLogType)
    {
        switch(aLogType)
        {
            case LogType::INFO:    return "INFO";
            case LogType::WARN:    return "WARN";
            case LogType::ERROR:     return "ERR!";
            case LogType::TRACE:   return "TRAC";
            default: return "----";
        }
    }

    template <class Duration>
    static inline std::string to_string(std::chrono::system_clock::time_point aTp)
    {
        std::stringstream lRet;
        lRet << std::chrono::time_point_cast<Duration>(aTp).time_since_epoch().count();
        return lRet.str();
    }

    static inline std::string to_string(std::string const &aFmt, std::chrono::system_clock::time_point aTp)
    {
        std::stringstream lRet;
        std::time_t lNow = std::chrono::system_clock::to_time_t(aTp);
        std::tm *lpTm = std::localtime(&lNow);
        lRet << std::put_time(lpTm, aFmt.data());
        return lRet.str();
    }

    static inline std::string format(char const *aFormat, va_list &aVars)
    {
        std::string lBuff;
        char str[MAX_BUF_SIZE];
        if (vsnprintf (str, sizeof str, aFormat, aVars) >= 0) lBuff = str;
        return std::move(lBuff);
    }

}; // class Util


struct LogInfo
{
    LogType _type;
    int _indent;
    std::chrono::system_clock::time_point _now;
    std::string _ctx;
    std::string _content;

    inline std::string to_string(std::string aSepa="\t") const
    {
        std::string lRet;
        lRet = Util::to_string<std::chrono::microseconds>(_now) \
                + aSepa + _ctx + aSepa \
                + Util::to_string(_type) + aSepa  \
                + std::to_string(_indent) + aSepa + /*std::string(_indent * 2, aSepa) +*/ _content + "\n";
        return lRet;
    }

    inline std::string to_json() const
    {
        std::string lRet;
        lRet = "{";
        lRet += "\"type\":" + std::to_string((uint8_t)_type) + ",";
        lRet += "\"indent\":" + std::to_string(_indent) + ",";
        lRet += "\"now\":" + Util::to_string<std::chrono::microseconds>(_now) + ",";
        lRet += "\"context\":\"" + _ctx + "\",";
        lRet += "\"content\":\"" + _content + "\"";
        lRet += "}";
        return lRet;
    }

};

class Export
{
public:
    virtual ~Export()=default;
    virtual void on_export(LogInfo const &aLogInfo)=0;
    virtual int on_init()=0;
    virtual void on_deinit()=0;
protected:
    std::mutex _mutex;
    uint8_t _init=0;
};

class EConsole : public Export
{
public:
    virtual int on_init();
    virtual void on_deinit();
    virtual void on_export(LogInfo const &aLogInfo);
};

class EFile : public Export
{
public:
    EFile(std::string const &aFile);
    EFile(std::string &&aFile);

    virtual int on_init();
    virtual void on_deinit();
    virtual void on_export(LogInfo const &aLogInfo);

protected:
    std::ofstream _ofs;
    std::string _f;
};

} // llt