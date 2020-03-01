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
#include <sys/socket.h>
#include <sys/un.h>

#include "utils.h"

#ifndef STATIC_LIB
#define UTIL_INLINE
#else
#define UTIL_INLINE inline
#endif


namespace llt {


struct LogInfo;
template <typename T> class Logger;

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
    static const std::string SEPARATOR;

    template <typename T>
    static inline std::string to_string(T const &aVal)
    {
        std::ostringstream ss;
        ss << aVal;
        return std::string(ss.str());
    }

    template <typename T>
    static inline std::string to_string_hex(T const &aVal)
    {
        std::ostringstream ss;
        ss << "0x" << std::hex << aVal;
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
        // std::string lBuff;
        char str[MAX_BUF_SIZE];
        if (vsnprintf (str, sizeof str, aFormat, aVars) >= 0) return str;
        return "";
        // return std::move(lBuff);
    }

}; // class Util


struct LogInfo
{
    LogType type_; //< log type
    size_t timestamp_; //< timestamp
    std::string tid_; //< process id, thread id
    std::string pid_; //< process id, thread id
    std::string logger_; //< address of logger
    std::string file_; //< file, function, line
    std::string func_; //< file, function, line
    std::string line_; //< file, function, line
    std::string message_; //< log message

    inline std::string to_string(std::string aSepa="\t") const
    {
        std::string ret;
        // ret = Util::to_string<std::chrono::microseconds>(_now) + aSepa \
        //         + pid_ + aSepa \
        //         + tid_ + aSepa \
        //         + Util::to_string_hex(reinterpret_cast<uintptr_t>(_this)) + aSepa \
        //         + Util::to_string(type_) + aSepa  \
        //         + std::to_string(_level) + aSepa \
        //         + _file + aSepa \
        //         + _function + aSepa \
        //         + Util::to_string(_line) + aSepa \
        //         + /*std::string(_level * 2, aSepa) +*/ content_ + "\n";
        return ret;
    }

    inline std::string toJson() const
    {
        std::string ret;
        // ret = "{";
        // ret += "\"now\":" + Util::to_string<std::chrono::microseconds>(_now) + ",";
        // ret += "\"type\":" + std::to_string((uint8_t)type_) + ",";
        // ret += "\"indent\":" + std::to_string(_level) + ",";
        // ret += "\"appName\":\"" + pid_ + "\",";
        // ret += "\"context\":\"" + tid_ + "\",";
        // ret += "\"content\":\"" + content_ + "\"";
        // ret += "},\n";
        return ret;
    }

};


template <typename T>
class Logger : public crtp<T, Logger>
{
public:
    // ~Logger()=default;
    void onLog(LogInfo const &log) { return this->underlying().onLog(log); }
    int onInit() { return this->underlying().onInit(); }
    void onDeinit() { return this->underlying().onDeinit(); }
protected:
    std::mutex mutex_;
    uint8_t init_=0;
};

class LConsole : public Logger<LConsole>
{
public:
    UTIL_INLINE int onInit();
    UTIL_INLINE void onDeinit();
    UTIL_INLINE void onLog(LogInfo const&);
};

class LFile : public Logger<LFile>
{
public:
    LFile(std::string file_name) : file_ (std::move(file_name)) {}
    ~LFile() {onDeinit();}

    UTIL_INLINE int onInit();
    UTIL_INLINE void onDeinit();
    UTIL_INLINE void onLog(LogInfo const&);

protected:
    std::ofstream ofs_;
    std::string file_;
};

class LNSClt : public Logger<LNSClt>
{
public:
    LNSClt(std::string sock_name) : sock_(std::move(sock_name)) {} ;
    ~LNSClt() { onDeinit(); }

    UTIL_INLINE int onInit();
    UTIL_INLINE void onDeinit();
    UTIL_INLINE void onLog(LogInfo const&);

protected:
    int fd_;
    std::string sock_;
    sockaddr_un svr_addr_;
};

class LogSystem
{
public:
    UTIL_INLINE size_t log(LogInfo const &);

protected:
    
};

} // llt


#ifndef STATIC_LIB
#include "exporter.cc"
#endif